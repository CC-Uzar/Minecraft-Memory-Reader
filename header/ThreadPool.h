#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>

// From https://www.geeksforgeeks.org/cpp/thread-pool-in-cpp/

class ThreadPool {
    private:
        std::vector<std::thread> threads;
        std::queue<std::function<void()>> tasks;
        std::mutex queue_mtx;
        std::condition_variable cv;
        bool stop = false;
        size_t thread_count;


    public:
        ThreadPool(size_t = std::thread::hardware_concurrency() / 2);
        ~ThreadPool();
        void enqueue(std::function<void()>);
        size_t getThreadCount();

        class InvalidThreadCount{};
        

};

#endif