#include "OD/pch.h"
#include "BatchRendererGroup.h"
#include <taskflow/algorithm/for_each.hpp>
#include <ankerl/unordered_dense.h>
#include <unordered_map>
#include <utility>

namespace OD {

struct BatchRendererGroup::Impl {
#ifndef OD_BRG_DISABLE_CHUNK_SYSTEM
    struct Chunk {
        AABB bounds;
        bool hasBounds = false;
        std::vector<uint32_t> instanceIndices;
    };
#endif

    struct Batch {
        uint32_t instanceBuffer = INVALID_RESOURCE_ID;
        std::vector<Instance> instances;
        CommandAccumulator commandAccumulator;
#ifndef OD_BRG_DISABLE_CHUNK_SYSTEM
        std::unordered_map<uint32_t, Chunk> chunks;
#endif
    };

    struct ChunkWork {
        BatchID batchId = INVALID_RESOURCE_ID;
        uint32_t chunkId = 0;
        Batch* batch = nullptr;
#ifndef OD_BRG_DISABLE_CHUNK_SYSTEM
        const Chunk* chunk = nullptr;
#endif
        std::vector<uint32_t> visibleInstances;
    };

    struct TaskflowState {
        Frustum view;
        DrawCommandList* output = nullptr;
        Impl* owner = nullptr;
        ChunkCullFunction cullChunk;
        InstanceCullFunction cullInstance;
        std::vector<ChunkWork> chunks;
    };

    uint64_t nextBatchId = 0;
    std::vector<std::unique_ptr<Batch>> batches;
#ifndef OD_BRG_DISABLE_CHUNK_SYSTEM
    bool chunkSystemEnabled = true;
#endif

#ifdef OD_BRG_DISABLE_CHUNK_SYSTEM
    explicit Impl(bool) {}
#else
    explicit Impl(bool enableChunkSystem) : chunkSystemEnabled(enableChunkSystem) {}
#endif

    Batch* GetBatch(BatchID id) {
        return id < batches.size() ? batches[id].get() : nullptr;
    }

    const Batch* GetBatch(BatchID id) const {
        return id < batches.size() ? batches[id].get() : nullptr;
    }

    static void AddVisibleInstance(
        BatchID batchId, Batch& batch, const Instance& instance, uint32_t instanceIndex
    ) {
        CommandAccumulator& accumulator = batch.commandAccumulator;
#ifndef OD_BRG_OMIT_BATCH_ID_FROM_COMMAND_KEY
        const CommandKey key{batchId, instance.meshId, instance.materialId};
#else
        const CommandKey key{instance.meshId, instance.materialId};
#endif
        auto [groupIt, inserted] = accumulator.groupIndices.try_emplace(key, static_cast<uint32_t>(accumulator.groups.size()));
        if(inserted) accumulator.groups.emplace_back();

        DrawCommandGroup& group = accumulator.groups[groupIt->second];
        group.command.batchId = batchId;
        group.command.meshId = instance.meshId;
        group.command.materialId = instance.materialId;
        group.command.instanceBuffer = batch.instanceBuffer;
        group.visibleInstanceIndices.push_back(instanceIndex);
        group.command.instanceCount = static_cast<uint32_t>(group.visibleInstanceIndices.size());
    }

    void WriteTo(DrawCommandList& output) const {
        output.Clear();
        size_t groupCount = 0;
        size_t totalVisible = 0;
        for (const auto& batch : batches) {
            if (!batch) continue;
            for (const DrawCommandGroup& group : batch->commandAccumulator.groups) {
                if (group.visibleInstanceIndices.empty()) continue;
                ++groupCount;
                totalVisible += group.visibleInstanceIndices.size();
            }
        }
        output.commands.reserve(groupCount);
        output.visibleInstances.reserve(totalVisible);

        for (const auto& batch : batches) {
            if (!batch) continue;
            for (const DrawCommandGroup& group : batch->commandAccumulator.groups) {
                if (group.visibleInstanceIndices.empty()) continue;
                DrawCommand command = group.command;
                command.visibleInstanceOffset = static_cast<uint32_t>(output.visibleInstances.size());
                command.instanceCount = static_cast<uint32_t>(group.visibleInstanceIndices.size());
                output.visibleInstances.insert(output.visibleInstances.end(),
                    group.visibleInstanceIndices.begin(), group.visibleInstanceIndices.end());
                output.commands.push_back(command);
            }
        }
    }

#ifndef OD_BRG_DISABLE_CHUNK_SYSTEM
    static void RecalculateChunkBounds(Chunk& chunk, const Batch& batch) {
        chunk.hasBounds = false;
        for (uint32_t instanceIndex : chunk.instanceIndices) {
            if (instanceIndex >= batch.instances.size()) continue;
            const AABB& bounds = batch.instances[instanceIndex].bounds;
            if (!chunk.hasBounds) {
                chunk.bounds = bounds;
                chunk.hasBounds = true;
            } else {
                chunk.bounds.Encapsulate(bounds);
            }
        }
    }
#endif
};

BatchRendererGroup::BatchRendererGroup(): BatchRendererGroup(true) {}
BatchRendererGroup::BatchRendererGroup(bool enableChunkSystem): impl(std::make_unique<Impl>(enableChunkSystem)) {}
BatchRendererGroup::~BatchRendererGroup() = default;
BatchRendererGroup::BatchRendererGroup(BatchRendererGroup&&) noexcept = default;
BatchRendererGroup& BatchRendererGroup::operator=(BatchRendererGroup&&) noexcept = default;

BatchID BatchRendererGroup::AddBatch(uint32_t bufferId) {
    if (!impl || impl->nextBatchId >= INVALID_RESOURCE_ID) {
        return INVALID_RESOURCE_ID;
    }

    const BatchID id = static_cast<BatchID>(impl->nextBatchId++);
    if (impl->batches.size() <= id) impl->batches.resize(static_cast<size_t>(id) + 1);
    auto batch = std::make_unique<Impl::Batch>();
    batch->instanceBuffer = bufferId;
    impl->batches[id] = std::move(batch);
    return id;
}

bool BatchRendererGroup::RemoveBatch(BatchID id) {
    if (!impl || id >= impl->batches.size() || !impl->batches[id]) return false;
    impl->batches[id].reset();
    return true;
}

bool BatchRendererGroup::SetBatchBuffer(BatchID id, uint32_t bufferId) {
    if (!impl) return false;
    Impl::Batch* batch = impl->GetBatch(id);
    if (!batch) return false;
    batch->instanceBuffer = bufferId;
    return true;
}

bool BatchRendererGroup::ReserveInstances(BatchID id, std::size_t capacity) {
    if (!impl) return false;
    Impl::Batch* batch = impl->GetBatch(id);
    if (!batch) return false;
    batch->instances.reserve(capacity);
    return true;
}

uint32_t BatchRendererGroup::AddInstance(
    BatchID batchId, uint32_t meshId, uint32_t materialId, uint32_t chunkId,
    const AABB& worldBounds
){
    if (!impl) return INVALID_RESOURCE_ID;
    Impl::Batch* batchPtr = impl->GetBatch(batchId);
    if (!batchPtr) return INVALID_RESOURCE_ID;
    Impl::Batch& batch = *batchPtr;
    if (batch.instances.size() >= INVALID_RESOURCE_ID) return INVALID_RESOURCE_ID;
    const uint32_t instanceIndex = static_cast<uint32_t>(batch.instances.size());

    Instance instance;
    instance.bounds = worldBounds;
    instance.meshId = meshId;
    instance.materialId = materialId;
    instance.chunkId = chunkId;
    batch.instances.push_back(std::move(instance));

#ifndef OD_BRG_DISABLE_CHUNK_SYSTEM
    if (impl->chunkSystemEnabled) {
        Impl::Chunk& chunk = batch.chunks[chunkId];
        chunk.instanceIndices.push_back(instanceIndex);
        if (!chunk.hasBounds) {
            chunk.bounds = worldBounds;
            chunk.hasBounds = true;
        } else {
            chunk.bounds.Encapsulate(worldBounds);
        }
    }
#endif

    return instanceIndex;
}

bool BatchRendererGroup::UpdateInstance(
    BatchID batchId, uint32_t instanceIndex, const AABB& worldBounds
){
    if (!impl) return false;
    Impl::Batch* batchPtr = impl->GetBatch(batchId);
    if (!batchPtr || instanceIndex >= batchPtr->instances.size()) return false;

    Impl::Batch& batch = *batchPtr;
    Instance& instance = batch.instances[instanceIndex];
    const uint32_t chunkId = instance.chunkId;
    instance.bounds = worldBounds;

#ifndef OD_BRG_DISABLE_CHUNK_SYSTEM
    if (impl->chunkSystemEnabled) {
        Impl::RecalculateChunkBounds(batch.chunks[chunkId], batch);
    }
#endif
    return true;
}

void BatchRendererGroup::BuildDrawCommands(
    const Frustum& view, DrawCommandList& output, const ChunkCullFunction& cullChunk,
    const InstanceCullFunction& cullInstance
) const {
    BuildDrawCommands(view, cullChunk, cullInstance);
    if (impl) impl->WriteTo(output);
    else output.Clear();
}

void BatchRendererGroup::BuildDrawCommands(
    const Frustum& view, const ChunkCullFunction& cullChunk,
    const InstanceCullFunction& cullInstance
) const {
    if (!impl) return;

    for (BatchID batchId = 0; batchId < impl->batches.size(); ++batchId) {
        Impl::Batch* batchPtr = impl->GetBatch(batchId);
        if (!batchPtr) continue;
        Impl::Batch& batch = *batchPtr;
        batch.commandAccumulator.ClearVisibleInstances();

        auto cullAndAddInstance = [&](uint32_t chunkId, uint32_t instanceIndex) {
            if (instanceIndex >= batch.instances.size()) return;
            const Instance& instance = batch.instances[instanceIndex];
            const bool instanceVisible = cullInstance ? cullInstance(batchId, chunkId, instanceIndex, instance, view) : instance.bounds.isOnFrustumSIMD(view);
            if (instanceVisible) Impl::AddVisibleInstance(batchId, batch, instance, instanceIndex);
        };

#ifdef OD_BRG_DISABLE_CHUNK_SYSTEM
        for (uint32_t instanceIndex = 0; instanceIndex < batch.instances.size(); ++instanceIndex) {
            cullAndAddInstance(batch.instances[instanceIndex].chunkId, instanceIndex);
        }
#else
        if (impl->chunkSystemEnabled) {
            for (const auto& [chunkId, chunk] : batch.chunks) {
                if (!chunk.hasBounds) continue;
                const bool chunkVisible = cullChunk ? cullChunk(batchId, chunkId, chunk.bounds, view) : chunk.bounds.isOnFrustumSIMD(view);
                if (!chunkVisible) continue;
                for (uint32_t instanceIndex : chunk.instanceIndices) {
                    cullAndAddInstance(chunkId, instanceIndex);
                }
            }
        } else {
            for (uint32_t instanceIndex = 0; instanceIndex < batch.instances.size(); ++instanceIndex) {
                cullAndAddInstance(batch.instances[instanceIndex].chunkId, instanceIndex);
            }
        }
#endif
    }

}

tf::Task BatchRendererGroup::BuildDrawCommandsTaskflow(
    tf::Taskflow& taskflow, const Frustum& view, DrawCommandList& output,
    const ChunkCullFunction& cullChunk,
    const InstanceCullFunction& cullInstance
) const{
    output.Clear();
    auto state = std::make_shared<Impl::TaskflowState>();
    state->view = view;
    state->output = &output;
    state->owner = impl.get();
    state->cullChunk = cullChunk;
    state->cullInstance = cullInstance;

    if (impl) {
        for (BatchID batchId = 0; batchId < impl->batches.size(); ++batchId) {
            Impl::Batch* batch = impl->GetBatch(batchId);
            if (!batch) continue;
            batch->commandAccumulator.ClearVisibleInstances();
#ifdef OD_BRG_DISABLE_CHUNK_SYSTEM
            Impl::ChunkWork work;
            work.batchId = batchId;
            work.batch = batch;
            state->chunks.push_back(std::move(work));
#else
            if (impl->chunkSystemEnabled) {
                for (const auto& [chunkId, chunk] : batch->chunks) {
                    if (!chunk.hasBounds) continue;
                    Impl::ChunkWork work;
                    work.batchId = batchId;
                    work.chunkId = chunkId;
                    work.batch = batch;
                    work.chunk = &chunk;
                    state->chunks.push_back(std::move(work));
                }
            } else {
                Impl::ChunkWork work;
                work.batchId = batchId;
                work.batch = batch;
                state->chunks.push_back(std::move(work));
            }
#endif
        }
    }

    // Each Taskflow worker owns one ChunkWork result, so culling never writes
    // into a shared vector. The dependent merge task is the only output writer.
    tf::Task cullTask = taskflow.emplace([state](tf::Subflow& subflow) {
        subflow.for_each_index(std::size_t{0}, state->chunks.size(), std::size_t{1}, [state](std::size_t workIndex) {
            Impl::ChunkWork& work = state->chunks[workIndex];
            const Impl::Batch& batch = *work.batch;

#ifndef OD_BRG_DISABLE_CHUNK_SYSTEM
            if (work.chunk) {
                const Impl::Chunk& chunk = *work.chunk;
                const bool chunkVisible = state->cullChunk
                    ? state->cullChunk(work.batchId, work.chunkId, chunk.bounds, state->view)
                    : chunk.bounds.isOnFrustumSIMD(state->view);
                if (!chunkVisible) return;
            }
#endif

            auto cullInstance = [&](uint32_t instanceIndex) {
                if (instanceIndex >= batch.instances.size()) return;
                const Instance& instance = batch.instances[instanceIndex];
#ifdef OD_BRG_DISABLE_CHUNK_SYSTEM
                const uint32_t chunkId = instance.chunkId;
#else
                const uint32_t chunkId = work.chunk ? work.chunkId : instance.chunkId;
#endif
                const bool visible = state->cullInstance
                    ? state->cullInstance(work.batchId, chunkId,
                        instanceIndex, instance, state->view)
                    : instance.bounds.isOnFrustumSIMD(state->view);
                if (visible) work.visibleInstances.push_back(instanceIndex);
            };

#ifdef OD_BRG_DISABLE_CHUNK_SYSTEM
            for (uint32_t instanceIndex = 0; instanceIndex < batch.instances.size(); ++instanceIndex) {
                cullInstance(instanceIndex);
            }
#else
            if (work.chunk) {
                for (uint32_t instanceIndex : work.chunk->instanceIndices) cullInstance(instanceIndex);
            } else {
                for (uint32_t instanceIndex = 0; instanceIndex < batch.instances.size(); ++instanceIndex) {
                    cullInstance(instanceIndex);
                }
            }
#endif
        });
    });

    tf::Task mergeTask = taskflow.emplace([state]() {
        for (const Impl::ChunkWork& work : state->chunks) {
            Impl::Batch& batch = *work.batch;
            for (uint32_t instanceIndex : work.visibleInstances) {
                const Instance& instance = batch.instances[instanceIndex];
                Impl::AddVisibleInstance(work.batchId, batch, instance, instanceIndex);
            }
        }

        if (state->owner) state->owner->WriteTo(*state->output);
        else state->output->Clear();
    });

    cullTask.precede(mergeTask);
    return mergeTask;
}

bool BatchRendererGroup::IsChunkSystemEnabled() const {
#ifdef OD_BRG_DISABLE_CHUNK_SYSTEM
    return false;
#else
    return impl && impl->chunkSystemEnabled;
#endif
}

const BatchRendererGroup::CommandAccumulator* BatchRendererGroup::GetCommandAccumulator(BatchID batchId) const {
    if (!impl) return nullptr;
    const Impl::Batch* batch = impl->GetBatch(batchId);
    return batch ? &batch->commandAccumulator : nullptr;
}

const BatchRendererGroup::CommandGroupIndexMap* BatchRendererGroup::GetGroupIndices(BatchID batchId) const {
    const CommandAccumulator* accumulator = GetCommandAccumulator(batchId);
    return accumulator ? &accumulator->groupIndices : nullptr;
}

const AABB* BatchRendererGroup::GetChunkBounds(BatchID batchId, uint32_t chunkId) const {
#ifdef OD_BRG_DISABLE_CHUNK_SYSTEM
    (void)batchId;
    (void)chunkId;
    return nullptr;
#else
    if (!impl) return nullptr;
    const Impl::Batch* batch = impl->GetBatch(batchId);
    if (!batch) return nullptr;
    const auto chunkIt = batch->chunks.find(chunkId);
    return chunkIt != batch->chunks.end() && chunkIt->second.hasBounds
        ? &chunkIt->second.bounds
        : nullptr;
#endif
}

const std::vector<BatchRendererGroup::Instance>* BatchRendererGroup::GetBatchInstances(BatchID id) const {
    if (!impl) return nullptr;
    const Impl::Batch* batch = impl->GetBatch(id);
    return batch ? &batch->instances : nullptr;
}

} // namespace OD
