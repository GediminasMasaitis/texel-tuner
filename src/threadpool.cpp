#include "threadpool.h"

#include <cstdint>
#include <thread>

using namespace std;

void ThreadPool::start(uint32_t thread_count)
{
    stop();
    should_stop = false;
    for (int thread_index = 0; thread_index < thread_count; thread_index++)
    {
        threads.emplace_back([this]()
        {
            thread_loop();
        });
    }
}

uint32_t ThreadPool::thread_count() const
{
    return static_cast<uint32_t>(threads.size());
}

void ThreadPool::enqueue(const function<void()>& job)
{
    {
        unique_lock<mutex> lock(queue_mutex);
        jobs.push(job);
    }
    mutex_condition.notify_one();
}

void ThreadPool::stop()
{
    {
        unique_lock<mutex> lock(queue_mutex);
        should_stop = true;
    }

    mutex_condition.notify_all();

    for (thread& active_thread : threads)
    {
        active_thread.join();
    }
    threads.clear();
}

bool ThreadPool::is_idle()
{
    unique_lock<mutex> lock(queue_mutex);
    return jobs.empty() && running_job_count == 0;
}

void ThreadPool::wait_for_completion()
{
    unique_lock<mutex> lock(queue_mutex);
    while(!jobs.empty() || running_job_count > 0)
    {
        completion_condition.wait(lock, [this]
        {
            return jobs.empty() && running_job_count == 0;
        });
    }
}

void ThreadPool::thread_loop()
{
    while (true)
    {
        function<void()> job;
        {
            unique_lock<mutex> lock(queue_mutex);
            mutex_condition.wait(lock, [this]
            {
                return !jobs.empty() || should_stop;
            });

            if (should_stop)
            {
                return;
            }

            job = jobs.front();
            jobs.pop();
            running_job_count++;
        }

        job();

        {
            unique_lock<mutex> lock(queue_mutex);
            running_job_count--;
            completion_condition.notify_all();
        }
    }
}
