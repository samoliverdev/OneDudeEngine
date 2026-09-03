#pragma once
#include "OD/Gfx/Gfx.h"
#include "OD/Platform/Platform.h"
#include <thread>
#include <mutex>
#include <functional>

namespace OD{
namespace Gfx{    

struct MultithreadRendererContext{
    std::atomic<bool> running = false;

    Gfx::RenderFrame frames[2];

    Gfx::RenderFrame* simulationFrame = nullptr; // Owned by main thread.
    Gfx::RenderFrame* renderFrame = nullptr; // Owned by render thread.

    std::thread renderThread;

    std::mutex mutex;
    std::condition_variable condition;

    bool renderRequested = false;
    bool renderFinished = false;

    std::function<void()> init; 
    std::function<void()> shut; 
    std::function<void(RenderFrame&)> runRender;

    void StartupFrames(){
        simulationFrame = &frames[0];
        renderFrame = &frames[1];
    }

    void Init(){
        running = true;
        Platform::StopCurrentContext();
        renderThread = std::thread([&]{ RenderThreadLoop(); });
    }

    void Shut(){
        running = false;
        condition.notify_all();
        if(renderThread.joinable()) renderThread.join();
    }

    void StartRender(){
        {
            std::lock_guard<std::mutex> lock(mutex);
            renderRequested = true;
        }
        condition.notify_one();
    }

    void WaitForRender(){
        std::unique_lock<std::mutex> lock(mutex);
        condition.wait(lock, [&]{ return renderFinished || !running; });
        renderFinished = false;
    }

    void SwapRenderFrames(){
        std::swap(simulationFrame, renderFrame);
    }

    RenderFrame& GetRenderFrame(){
        return *simulationFrame;
    }

    void RenderThreadLoop(){
        Platform::MakeMultiThreadContext();

        init();

        while(running){
            
            // Wait for this frame's render work.
            {
                std::unique_lock<std::mutex> lock(mutex);

                condition.wait(
                    lock,
                    [&]{
                        return renderRequested || !running;
                    }
                );

                if(!running) return;

                renderRequested = false;
            }

            // ------------------------------------------------
            // Execute render commands.
            //
            // This happens on the render thread.
            // ------------------------------------------------

            //RunRender(*renderFrame);
            {
                //SimpleTimer s([](float t){ LogInfo("GpuTime: {}", t); });
                runRender(*renderFrame);
            }

            // ------------------------------------------------
            // Tell main thread that render is complete.
            // ------------------------------------------------

            {
                std::lock_guard<std::mutex> lock(mutex);
                renderFinished = true;
            }

            condition.notify_one();
        }

        shut();
    }
};

}
}