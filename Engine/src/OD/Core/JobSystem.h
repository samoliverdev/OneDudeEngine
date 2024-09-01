#pragma once
#include "OD/Defines.h"
#include <functional>
#include <stdint.h>

#include <algorithm>    // std::max
#include <atomic>    // to use std::atomic<uint64_t>
#include <thread>    // to use std::thread
#include <condition_variable>    // to use std::condition_variable
#include <sstream>
#include <assert.h>

//Source: https://github.com/turanszkij/JobSystem

namespace OD{

// A Dispatched job will receive this as function argument:
struct OD_API JobDispatchArgs{
	uint32_t jobIndex;
	uint32_t groupIndex;
};

namespace JobSystem{

// Create the internal resources such as worker threads, etc. Call it once when initializing the application.
void OD_API Initialize();

// Add a job to execute asynchronously. Any idle thread will execute this job.
void OD_API Execute(const std::function<void()>& job);

// Divide a job onto multiple jobs and execute in parallel.
//	jobCount	: how many jobs to generate for this task.
//	groupSize	: how many jobs to execute per thread. Jobs inside a group execute serially. It might be worth to increase for small jobs
//	func		: receives a JobDispatchArgs as parameter
void OD_API Dispatch(uint32_t jobCount, uint32_t groupSize, const std::function<void(JobDispatchArgs)>& job);

// Check if any threads are working currently or not
bool OD_API IsBusy();

// Wait until all threads become idle
void OD_API Wait();

}
}