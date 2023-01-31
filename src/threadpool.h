#ifndef THREADPOOL_H
#define THREADPOOL_H 1

#include <functional>
#include <mutex>
#include <queue>

class ThreadPool {
public:
    void start(uint32_t thread_count);
    uint32_t thread_count() const;
    void enqueue(const std::function<void()>& job);
    void stop();
    bool is_idle();
    void wait_for_completion();

private:
    bool should_stop = false;
    uint32_t running_job_count = 0;
    std::mutex queue_mutex;
    std::condition_variable mutex_condition;
    std::condition_variable completion_condition;
    std::vector<std::thread> threads;
    std::queue<std::function<void()>> jobs;

    void thread_loop();
};

#endif // !THREADPOOL_H
