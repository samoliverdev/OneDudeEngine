#pragma once

// CPU-only BatchRendererGroup sketch. It manages batch data and builds
// visibility-indexed draw commands, but does not allocate or submit GPU work.

#define OD_BRG_OMIT_BATCH_ID_FROM_COMMAND_KEY
//#define OD_BRG_DISABLE_CHUNK_SYSTEM

#include <OD/Defines.h>
#include <OD/Graphics/Culling.h>
#include <OD/Core/Resource.h>
#include <ankerl/unordered_dense.h>
#include <taskflow/taskflow.hpp>
#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

namespace OD {

using BatchID = uint32_t;

class OD_API BatchRendererGroup {
public:
    // Threading contract: parallel AddInstance calls are allowed for distinct,
    // pre-created BatchIDs, with exactly one writer per ID. No operation may
    // resize/remove batches, read a batch, or write the same batch until those
    // writers finish. BRG uses no mutex here; the standard allocator may still
    // synchronize internally when its containers grow.
    struct Instance {
        AABB bounds;
        uint32_t meshId = INVALID_RESOURCE_ID;
        uint32_t materialId = INVALID_RESOURCE_ID;
        uint32_t chunkId = 0;
    };

    struct DrawCommand {
        BatchID batchId = INVALID_RESOURCE_ID;
        uint32_t meshId = INVALID_RESOURCE_ID;
        uint32_t materialId = INVALID_RESOURCE_ID;
        uint32_t instanceBuffer = INVALID_RESOURCE_ID;
        uint32_t visibleInstanceOffset = 0;
        uint32_t instanceCount = 0;
    };

    struct CommandKey {
#ifndef OD_BRG_OMIT_BATCH_ID_FROM_COMMAND_KEY
        BatchID batchId = INVALID_RESOURCE_ID;
#endif
        uint32_t meshId = INVALID_RESOURCE_ID;
        uint32_t materialId = INVALID_RESOURCE_ID;

        bool operator==(const CommandKey& other) const noexcept {
#ifndef OD_BRG_OMIT_BATCH_ID_FROM_COMMAND_KEY
            return batchId == other.batchId && meshId == other.meshId && materialId == other.materialId;
#else
            return meshId == other.meshId && materialId == other.materialId;
#endif
        }
    };

    struct CommandKeyHash {
        size_t operator()(const CommandKey& key) const noexcept {
#ifndef OD_BRG_OMIT_BATCH_ID_FROM_COMMAND_KEY
            size_t hash = std::hash<uint32_t>{}(key.batchId);
#else
            size_t hash = 0;
#endif
            hash ^= std::hash<uint32_t>{}(key.meshId) + size_t{0x9e3779b9} + (hash << 6) + (hash >> 2);
            hash ^= std::hash<uint32_t>{}(key.materialId) + size_t{0x9e3779b9} + (hash << 6) + (hash >> 2);
            return hash;
        }
    };

    using CommandGroupIndexMap = ankerl::unordered_dense::map<CommandKey, uint32_t, CommandKeyHash>;

    struct DrawCommandGroup {
        DrawCommand command;
        std::vector<uint32_t> visibleInstanceIndices;
    };

    // Persistent per-batch grouping state. ClearVisibleInstances retains the
    // map, group vector, and each group's visible-index capacity across builds.
    struct CommandAccumulator {
        std::vector<DrawCommandGroup> groups;
        CommandGroupIndexMap groupIndices;

        void ClearVisibleInstances() {
            for (DrawCommandGroup& group : groups) {
                group.visibleInstanceIndices.clear();
                group.command.visibleInstanceOffset = 0;
                group.command.instanceCount = 0;
            }
        }
    };

    struct DrawCommandList {
        std::vector<DrawCommand> commands;
        // Indices are local to the instance data of the BatchID on each command.
        std::vector<uint32_t> visibleInstances;

        // Clear while keeping capacity for reuse across views and frames.
        void Clear() {
            commands.clear();
            visibleInstances.clear();
        }
    };

    using ChunkCullFunction = std::function<bool(BatchID batchId, uint32_t chunkId, const AABB& chunkBounds, const Frustum& view)>;
    using InstanceCullFunction = std::function<bool(BatchID batchId, uint32_t chunkId, uint32_t instanceIndex, const Instance& instance, const Frustum& view)>;

    BatchRendererGroup();
    explicit BatchRendererGroup(bool enableChunkSystem);
    ~BatchRendererGroup();

    BatchRendererGroup(const BatchRendererGroup&) = delete;
    BatchRendererGroup& operator=(const BatchRendererGroup&) = delete;
    BatchRendererGroup(BatchRendererGroup&&) noexcept;
    BatchRendererGroup& operator=(BatchRendererGroup&&) noexcept;

    // Create/remove batches outside parallel instance writes; these resize or
    // change the batch index table.
    BatchID AddBatch(uint32_t bufferId = INVALID_RESOURCE_ID);
    bool RemoveBatch(BatchID id);
    bool SetBatchBuffer(BatchID id, uint32_t bufferId);
    // Reserve before parallel AddInstance calls begin for this batch.
    bool ReserveInstances(BatchID id, std::size_t capacity);

    // No-lock parallel writes are safe when each BatchID has exactly one writer.
    // Different batch IDs own separate storage; do not write one batch from two threads.
    // Finish all writers before culling, reading instance data, or changing batches.
    uint32_t AddInstance(
        BatchID batchId,
        uint32_t meshId,
        uint32_t materialId,
        uint32_t chunkId,
        const AABB& worldBounds
    );

    bool UpdateInstance(
        BatchID batchId,
        uint32_t instanceIndex,
        const AABB& worldBounds
    );

    // Build calls on the same group reuse its per-batch accumulators and must
    // not overlap each other. Complete all Taskflow work before the next build.
    void BuildDrawCommands(
        const Frustum& view,
        DrawCommandList& output,
        const ChunkCullFunction& cullChunk = {},
        const InstanceCullFunction& cullInstance = {}
    ) const;

    // Updates each batch's persistent CommandAccumulator without flattening
    // its groups into a DrawCommandList. Iterate groups with non-empty
    // visibleInstanceIndices; groupIndices maps a key to its index in groups.
    void BuildDrawCommands(
        const Frustum& view,
        const ChunkCullFunction& cullChunk = {},
        const InstanceCullFunction& cullInstance = {}
    ) const;

    // Adds parallel chunk/instance culling plus a dependent merge task to the
    // caller's graph. The caller owns the graph and output, and must not mutate
    // this group until the returned task has completed.
    tf::Task BuildDrawCommandsTaskflow(
        tf::Taskflow& taskflow,
        const Frustum& view,
        DrawCommandList& output,
        const ChunkCullFunction& cullChunk = {},
        const InstanceCullFunction& cullInstance = {}
    ) const;

    bool IsChunkSystemEnabled() const;
    // These pointers remain valid until their batch is removed or the BRG is
    // moved/destroyed. Do not mutate groupIndices; it stores internal indices.
    const CommandAccumulator* GetCommandAccumulator(BatchID batchId) const;
    const CommandGroupIndexMap* GetGroupIndices(BatchID batchId) const;
    const AABB* GetChunkBounds(BatchID batchId, uint32_t chunkId) const;
    const std::vector<Instance>* GetBatchInstances(BatchID id) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace OD
