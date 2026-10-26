#pragma once

// MiniJobs: a small Taskflow-backed job dependency scheduler.
// C++20; deliberately independent of ECS and component storage.
#include <taskflow/taskflow.hpp>

#include <algorithm>
#include <cstddef>
//#include <concepts>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mini_jobs{

struct Resource{
    const void* key;
    explicit Resource(const void* p): key(p){
        if (!p) throw std::invalid_argument("Resource key cannot be null");
    }

    // Use the actual shared object's address as the resource identity.
    template<class T>
    static Resource Of(const T& object){
        return Resource(std::addressof(object));
    }

    // A logical resource with no backing object (e.g. a component type).
    // All calls with the same Tag type return the same resource identity.
    template<class Tag>
    static Resource TagOf(){
        static const unsigned char token = 0;
        return Resource(&token);
    }
};

struct Access{
    std::vector<const void*> reads;
    std::vector<const void*> writes;

    Access& Read(Resource r){ reads.push_back(r.key); return *this; }
    Access& Write(Resource r){ writes.push_back(r.key); return *this; }

    // Type tags share an identity per type, independent of EnTT.
    // A read/write declaration is metadata, not a memory safety guarantee.
    template<class... T> //requires (sizeof...(T) > 0)
    Access& Read(){
        (Read(Resource::TagOf<T>()), ...);
        return *this;
    }

    template<class... T> //requires (sizeof...(T) > 0)
    Access& Write(){
        (Write(Resource::TagOf<T>()), ...);
        return *this;
    }
};

// Unity-style job interfaces. Override Execute(); scheduling stores the job
// object and invokes Execute later, from a Taskflow worker.
struct IJob{
    virtual ~IJob() = default;
    virtual void Execute() = 0;
};

// One Execute(begin,end) call per chunk. Chunks can execute concurrently.
struct IJobParallelFor{
    virtual ~IJobParallelFor() = default;
    virtual void Execute(std::size_t begin, std::size_t end) = 0;
};

struct JobHandle{
    std::size_t index = static_cast<std::size_t>(-1);
    std::uint64_t generation = 0;
};

/*template<class T>
struct IsJobPtr: std::false_type{};

template<class T>
struct IsJobPtr<std::shared_ptr<T>>: std::bool_constant<std::derived_from<T, IJob>>{};*/

class JobSystem{
public:
    explicit JobSystem(tf::Executor& executor): executor_(executor){}

    // Copies/moves a Unity-style job into owned storage. Nothing in
    // Execute() runs until Run(). EnTT views may be constructed there.
    template<class T> //requires std::derived_from<std::decay_t<T>, IJob>
    JobHandle ScheduleJob(T&& job, Access access = {}){
        using Job = std::decay_t<T>;
        auto owned = std::make_shared<Job>(std::forward<T>(job));
        return Add([owned](tf::Subflow&){ owned->Execute(); }, std::move(access));
    }

    // Same semantics, for polymorphic / non-copyable jobs. Caller can
    // hold the shared_ptr, but scheduler keeps the object alive.
    template<class T> //requires std::derived_from<T, IJob>
    JobHandle ScheduleJob(std::shared_ptr<T> job, Access access = {}){
        if(!job) throw std::invalid_argument("null job");
        return Add([job = std::move(job)](tf::Subflow&){
            job->Execute();
        }, std::move(access));
    }

    template<class T> //requires std::derived_from<std::decay_t<T>, IJobParallelFor>
    JobHandle ScheduleParallelJob(T&& job, std::size_t count, std::size_t grain, Access access = {}){
        using Job = std::decay_t<T>;
        return ScheduleParallel(
            count, grain,
            [owned = std::make_shared<Job>(std::forward<T>(job))]
            (std::size_t begin, std::size_t end) {
                owned->Execute(begin, end);
            }, std::move(access)
        );
    }

    template<class T> //requires std::derived_from<T, IJobParallelFor>
    JobHandle ScheduleParallelJob(std::shared_ptr<T> job, std::size_t count, std::size_t grain, Access access = {}){
        if(!job) throw std::invalid_argument("null job");
        return ScheduleParallel(count, grain,
            [job = std::move(job)](std::size_t begin, std::size_t end){
                job->Execute(begin, end);
            }, std::move(access)
        );
    }

    // Advanced job: exposes a Taskflow subflow, allowing work counts and
    // entity views to be determined at EXECUTION time instead of at Schedule.
    template<class Fn>
    JobHandle ScheduleSubflow(Fn&& fn, Access access = {}){
        return Add(std::function<void(tf::Subflow&)>(std::forward<Fn>(fn)), std::move(access));
    }

    // Jobs are collected on one thread, then submitted as a DAG.
    // A job may freely access arbitrary data, but must declare conflicting access.
    template<class Fn> //requires (!std::derived_from<std::decay_t<Fn>, IJob> && !IsJobPtr<std::decay_t<Fn>>::value)
    JobHandle Schedule(Fn&& fn, Access access = {}){
        return Add(std::function<void(tf::Subflow&)>(
            [work = std::forward<Fn>(fn)](tf::Subflow&) mutable { work(); }
        ), std::move(access));
    }

    // body(begin, end) is called once per chunk of [0, count).
    template<class Fn>
    JobHandle ScheduleParallel(std::size_t count, std::size_t grain, Fn&& body, Access access = {}){
        if(grain == 0) throw std::invalid_argument("grain must be > 0");
        auto exec = [count, grain, work = std::forward<Fn>(body)](tf::Subflow& sf) mutable {
            const std::size_t chunks = count / grain + (count % grain != 0);
            sf.for_each_index(std::size_t{0}, chunks, std::size_t{1},
                [count, grain, &work](std::size_t chunk) {
                    const std::size_t begin = chunk * grain;
                    const std::size_t end = begin + std::min(grain, count - begin);
                    work(begin, end);
                });
            // The default subflow join prevents successor jobs starting early.
        };
        return Add(std::function<void(tf::Subflow&)>(std::move(exec)), std::move(access));
    }

    // 'after' does not begin until 'before' and all its child tasks are done.
    void DependsOn(JobHandle after, JobHandle before){
        Validate(after);
        Validate(before);
        if (after.index == before.index) throw std::invalid_argument("a job cannot depend on itself");
        // Edges always go from an earlier scheduled job to a later one;
        // this avoids cycles and matches the automatic access ordering.
        if (before.index > after.index) throw std::invalid_argument("dependency must refer to an earlier job");
        nodes_[after.index].predecessors.push_back(before.index);
    }

    // Execute the queued DAG, wait for all jobs, and clear the batch.
    // Schedule() never executes work; Run() is the synchronization point.
    void Run(){
        tf::Taskflow flow;
        std::vector<tf::Task> tasks;
        tasks.reserve(nodes_.size());
        for(auto& node: nodes_){
            tasks.emplace_back(flow.emplace(std::move(node.execute)));
        }
        for(std::size_t i = 0; i < nodes_.size(); ++i){
            for(std::size_t predecessor : nodes_[i].predecessors){
                tasks[predecessor].precede(tasks[i]);
            }
        }
        // The graph owns all scheduled work and stays alive until wait completes.
        // Clear queued metadata even if a task throws through future.get().
        nodes_.clear();
        resources_.clear();
        ++generation_;
        executor_.run(flow).get();
    }

private:
    struct Node{
        std::function<void(tf::Subflow&)> execute;
        std::vector<std::size_t> predecessors;
    };
    struct ResourceState{
        std::size_t writer = invalid;
        std::vector<std::size_t> readers;
    };
    static constexpr std::size_t invalid = static_cast<std::size_t>(-1);

    tf::Executor& executor_;
    std::vector<Node> nodes_;
    std::unordered_map<const void*, ResourceState> resources_;
    std::uint64_t generation_ = 1;

    void Validate(JobHandle h) const {
        if(h.generation != generation_ || h.index >= nodes_.size()) throw std::invalid_argument("stale or foreign JobHandle");
    }

    JobHandle Add(std::function<void(tf::Subflow&)> execute, Access access) {
        const std::size_t id = nodes_.size();
        Node node;
        node.execute = std::move(execute);
        auto addPredecessor = [&](std::size_t p){
            if (p != invalid && p != id) node.predecessors.push_back(p);
        };
        // A write takes precedence over a read when the same key appears in both.
        for(const auto* key : access.writes){
            if(!key) throw std::invalid_argument("null write resource");
        }
        for(const auto* key : access.reads){
            if(!key) throw std::invalid_argument("null read resource");
        }
        for(const auto* key : access.reads){
            if(std::find(access.writes.begin(), access.writes.end(), key) != access.writes.end())
                continue;
            auto& r = resources_[key];
            addPredecessor(r.writer);
            r.readers.push_back(id);
        }
        for(const auto* key : access.writes){
            auto& r = resources_[key];
            addPredecessor(r.writer);
            for(auto reader: r.readers) addPredecessor(reader);
            r.readers.clear();
            r.writer = id;
        }
        auto& p = node.predecessors;
        std::sort(p.begin(), p.end());
        p.erase(std::unique(p.begin(), p.end()), p.end());
        nodes_.push_back(std::move(node));
        return {id, generation_};
    }
};

} // namespace mini_jobs
