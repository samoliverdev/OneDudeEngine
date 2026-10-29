#include "OD/pch.h"
#include <OD/Core/Instrumentor.h>
#include "BatchRendererGroup.h"

namespace {

OD::AABB MakeInstanceBounds(const OD::Vector3& position) {
    return OD::AABB(position, 0.5f, 0.5f, 0.5f);
}

void RunBatchRendererGroupPerformanceTest(tf::Executor& executor){
    using namespace OD;
    constexpr uint32_t batchCount = 4;
    constexpr uint32_t instancesPerBatch = 8192;
    constexpr uint32_t instancesPerChunk = 128;
    constexpr int measuredIterations = 25;

    BatchRendererGroup performanceBatch;
    std::vector<BatchID> batchIds(batchCount);
    for (uint32_t batchIndex = 0; batchIndex < batchCount; ++batchIndex) {
        batchIds[batchIndex] = performanceBatch.AddBatch(4000 + batchIndex);
        performanceBatch.ReserveInstances(batchIds[batchIndex], instancesPerBatch);
    }

    tf::Taskflow instanceGenerationTaskflow;
    float instanceGenerationMs = 0.0f;
    {
        SimpleTimer timer([&](float elapsedMs) { instanceGenerationMs = elapsedMs; });
        for (uint32_t batchIndex = 0; batchIndex < batchCount; ++batchIndex) {
            instanceGenerationTaskflow.emplace([&, batchIndex]() {
                const BatchID batchId = batchIds[batchIndex];
                for (uint32_t instanceIndex = 0; instanceIndex < instancesPerBatch; ++instanceIndex) {
                    const uint32_t chunkId = instanceIndex / instancesPerChunk;
                    const Vector3 position(
                        static_cast<float>(instanceIndex % instancesPerChunk),
                        static_cast<float>(chunkId),
                        static_cast<float>(batchIndex)
                    );
                    const uint32_t meshId = instanceIndex % 32;
                    const uint32_t materialId = (instanceIndex / 32) % 8;
                    performanceBatch.AddInstance(batchId, meshId, materialId, chunkId, MakeInstanceBounds(position));
                }
            });
        }
        executor.run(instanceGenerationTaskflow).get();
    }

    LogInfo("BRG performance: generated {} instances in {:.3f} ms",
        batchCount * instancesPerBatch, instanceGenerationMs);
#ifdef OD_BRG_OMIT_BATCH_ID_FROM_COMMAND_KEY
    LogInfo("BRG command key benchmark mode: BatchID omitted (grouping scoped per batch)");
#else
    LogInfo("BRG command key benchmark mode: BatchID included");
#endif

    Frustum benchmarkView;
    benchmarkView.leftFace = Plane(glm::vec4(1.0f, 0.0f, 0.0f, 2.0f));
    benchmarkView.rightFace = Plane(glm::vec4(-1.0f, 0.0f, 0.0f, 31.0f));
    benchmarkView.bottomFace = Plane(glm::vec4(0.0f, 1.0f, 0.0f, 2.0f));
    benchmarkView.topFace = Plane(glm::vec4(0.0f, -1.0f, 0.0f, 31.0f));
    benchmarkView.nearFace = Plane(glm::vec4(0.0f, 0.0f, 1.0f, 2.0f));
    benchmarkView.farFace = Plane(glm::vec4(0.0f, 0.0f, -1.0f, 4.0f));
    const BatchRendererGroup::ChunkCullFunction passChunk = [](BatchID, uint32_t, const AABB& chunkBounds, const Frustum& view){
        return chunkBounds.isOnFrustumSIMD(view);
    };
    const BatchRendererGroup::InstanceCullFunction passInstance = [](BatchID, uint32_t, uint32_t, const BatchRendererGroup::Instance& instance, const Frustum& view){
        return instance.bounds.isOnFrustumSIMD(view);
    };

    BatchRendererGroup::DrawCommandList synchronousCommands;
    performanceBatch.BuildDrawCommands(benchmarkView, synchronousCommands, passChunk, passInstance);
    float synchronousTotalMs = 0.0f;
    {
        SimpleTimer timer([&](float elapsedMs) { synchronousTotalMs = elapsedMs; });
        for (int iteration = 0; iteration < measuredIterations; ++iteration) {
            performanceBatch.BuildDrawCommands(benchmarkView, synchronousCommands, passChunk, passInstance);
        }
    }
    LogInfo(
        "BRG sync command generation: avg {:.3f} ms over {} iterations; commands={} visible={}",
        synchronousTotalMs / measuredIterations, measuredIterations,
        synchronousCommands.commands.size(), synchronousCommands.visibleInstances.size()
    );

    BatchRendererGroup::DrawCommandList taskflowCommands;
    tf::Taskflow cullingTaskflow;
    const tf::Task taskflowBuild = performanceBatch.BuildDrawCommandsTaskflow(cullingTaskflow, benchmarkView, taskflowCommands, passChunk, passInstance);
    executor.run(cullingTaskflow).get();
    cullingTaskflow.clear();
    (void)taskflowBuild;

    float taskflowTotalMs = 0.0f;
    {
        SimpleTimer timer([&](float elapsedMs) { taskflowTotalMs = elapsedMs; });
        for (int iteration = 0; iteration < measuredIterations; ++iteration) {
            cullingTaskflow.clear();
            const tf::Task build = performanceBatch.BuildDrawCommandsTaskflow(cullingTaskflow, benchmarkView, taskflowCommands, passChunk, passInstance);
            executor.run(cullingTaskflow).get();
            (void)build;
        }
    }
    LogInfo(
        "BRG Taskflow command generation: avg {:.3f} ms over {} iterations; commands={} visible={}",
        taskflowTotalMs / measuredIterations, measuredIterations,
        taskflowCommands.commands.size(), taskflowCommands.visibleInstances.size()
    );
}

void RunBatchRendererGroupParallelAddTest(tf::Executor& executor) {
    using namespace OD;
    constexpr uint32_t batchCount = 4;
    constexpr uint32_t instancesPerBatch = 512;

    BatchRendererGroup testBatch;
    std::vector<BatchID> batchIds(batchCount);
    std::vector<uint32_t> addedCounts(batchCount, 0);
    for (uint32_t batchIndex = 0; batchIndex < batchCount; ++batchIndex) {
        batchIds[batchIndex] = testBatch.AddBatch(5000 + batchIndex);
    }

    tf::Taskflow addTasks;
    float parallelAddMs = 0.0f;
    {
        SimpleTimer timer([&](float elapsedMs) { parallelAddMs = elapsedMs; });
        for (uint32_t batchIndex = 0; batchIndex < batchCount; ++batchIndex) {
            addTasks.emplace([&, batchIndex]() {
                const BatchID batchId = batchIds[batchIndex];
                for (uint32_t instanceIndex = 0; instanceIndex < instancesPerBatch; ++instanceIndex) {
                    const uint32_t chunkId = instanceIndex / 64;
                    const Vector3 position(
                        static_cast<float>(instanceIndex % 64),
                        static_cast<float>(chunkId),
                        static_cast<float>(batchIndex));
                    const uint32_t result = testBatch.AddInstance(
                        batchId, 1, 2, chunkId, MakeInstanceBounds(position));
                    if (result != INVALID_RESOURCE_ID) ++addedCounts[batchIndex];
                }
            });
        }
        executor.run(addTasks).get();
    }
    LogInfo("BRG parallel AddInstance test: generated {} instances in {:.3f} ms",
        batchCount * instancesPerBatch, parallelAddMs);

    bool passed = true;
    for (uint32_t batchIndex = 0; batchIndex < batchCount; ++batchIndex) {
        const auto* instances = testBatch.GetBatchInstances(batchIds[batchIndex]);
        const bool batchPassed = instances
            && instances->size() == instancesPerBatch
            && addedCounts[batchIndex] == instancesPerBatch;
        passed = passed && batchPassed;
        LogInfo("BRG parallel AddInstance test: batch={} added={} stored={} result={}",
            batchIds[batchIndex], addedCounts[batchIndex],
            instances ? instances->size() : 0, batchPassed ? "PASS" : "FAIL");
    }
    LogInfo("BRG parallel AddInstance test overall: {}", passed ? "PASS" : "FAIL");
}

}

void BatchRendererGroupSample::OnInit() {
    using namespace OD;
    LogInfo("BatchRendererGroup sample: generating CPU draw commands");

    const BatchID firstBatch = batch.AddBatch(1000);
    const BatchID secondBatch = batch.AddBatch(2000);
    const BatchID thirdBatch = batch.AddBatch(3000);

    // Repeated (mesh, material) IDs in different visible chunks become one command.
    batch.AddInstance(firstBatch, 10, 20, 10, MakeInstanceBounds(Vector3(-1, 0, 0)));
    batch.AddInstance(firstBatch, 10, 20, 10, MakeInstanceBounds(Vector3(1, 0, 0)));
    batch.AddInstance(firstBatch, 10, 20, 10, MakeInstanceBounds(Vector3(-2, 0, 0)));
    batch.AddInstance(firstBatch, 11, 20, 10, MakeInstanceBounds(Vector3(-1, 1, 0)));
    batch.AddInstance(firstBatch, 10, 20, 30, MakeInstanceBounds(Vector3(-3, 0, 0)));
    batch.AddInstance(secondBatch, 10, 20, 10, MakeInstanceBounds(Vector3(0, 0, 1)));
    batch.AddInstance(thirdBatch, 10, 20, 10, MakeInstanceBounds(Vector3(0, 1, 0)));

    // Chunk 20 is rejected by the custom CPU callback.
    batch.AddInstance(firstBatch, 10, 20, 20, MakeInstanceBounds(Vector3(100, 0, 0)));
    batch.AddInstance(secondBatch, 10, 20, 20, MakeInstanceBounds(Vector3(101, 0, 0)));
    batch.AddInstance(thirdBatch, 10, 20, 20, MakeInstanceBounds(Vector3(102, 0, 0)));

    const Frustum sampleView{};
    BatchRendererGroup::DrawCommandList commands;
    tf::Taskflow cullingTaskflow;
    tf::Executor executor;
    const tf::Task buildComplete = batch.BuildDrawCommandsTaskflow(
        cullingTaskflow, sampleView, commands,
        [](BatchID, uint32_t chunkId, const AABB&, const Frustum&) {
            return chunkId == 10 || chunkId == 30;
        },
        [](BatchID, uint32_t, uint32_t, const BatchRendererGroup::Instance& instance, const Frustum&) {
            return instance.bounds.center.x <= 0.0f;
        }
    );
    executor.run(cullingTaskflow).get();

    LogInfo("Generated draw command count: {}", commands.commands.size());
    for (const auto& command : commands.commands) {
        LogInfo(
            "DrawCommand batch={} mesh={} material={} visibleOffset={} visibleCount={} buffer={}",
            command.batchId, command.meshId, command.materialId,
            command.visibleInstanceOffset, command.instanceCount, command.instanceBuffer
        );
        const uint32_t visibleEnd = command.visibleInstanceOffset + command.instanceCount;
        for (uint32_t visibleIndex = command.visibleInstanceOffset; visibleIndex < visibleEnd; ++visibleIndex) {
            LogInfo("  visible batch buffer index={}", commands.visibleInstances[visibleIndex]);
        }
    }

    RunBatchRendererGroupParallelAddTest(executor);
    RunBatchRendererGroupPerformanceTest(executor);
}

void BatchRendererGroupSample::OnUpdate(float) {}
void BatchRendererGroupSample::OnRender(float) {}
void BatchRendererGroupSample::OnGUI() {}
void BatchRendererGroupSample::OnResize(int, int) {}
void BatchRendererGroupSample::OnExit() {}
