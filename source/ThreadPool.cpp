#include "ThreadPool.h"
#include <iostream>

ThreadPool::ThreadPool(size_t t) {
    thread_count = t;
    for (size_t i = 0; i < t; i++) {
        threads.emplace_back([this] {
            while (true) {
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(queue_mtx);

                    cv.wait(lock, [this] {
                        return !tasks.empty() || stop;
                    });

                    if (stop && tasks.empty()) return;

                    task = std::move(tasks.front());
                    tasks.pop();
                }

                task();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mtx);
        stop = true;
    }
    cv.notify_all();

    for (auto& thread : threads)
        thread.join();
}

void ThreadPool::enqueue(std::function<void()> t) {
    {
        std::unique_lock<std::mutex> lock(queue_mtx);
        tasks.emplace(std::move(t));
    }
    cv.notify_one();
}

size_t ThreadPool::getThreadCount() {
    return thread_count;
}