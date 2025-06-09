#ifndef __NOTIFY_HH__
#define __NOTIFY_HH__
#include <semaphore.h>
#include <unistd.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

template <typename T>
class ThreadsafeQueue {
    std::queue<T> queue_;
    mutable std::mutex mutex_;

    // Moved out of public interface to prevent races between this
    // and pop().

   public:
    ThreadsafeQueue() = default;
    ThreadsafeQueue(const ThreadsafeQueue<T> &) = delete;
    ThreadsafeQueue &operator=(const ThreadsafeQueue<T> &) = delete;

    ThreadsafeQueue(ThreadsafeQueue<T> &&other)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_ = std::move(other.queue_);
    }

    virtual ~ThreadsafeQueue() {}

    unsigned long Size() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    T Pop()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return {};
        }
        T tmp = queue_.front();
        queue_.pop();
        return tmp;
    }

    void Push(const T &item)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(item);
    }
    size_t size() const { return queue_.size(); }
    bool empty() const { return queue_.empty(); }
};

template <typename T>
class Publisher {
   public:
    Publisher(std::shared_ptr<ThreadsafeQueue<T>> queue, sem_t *sem)
        : queue_(queue), semphore_(sem)
    {
    }

   protected:
    std::shared_ptr<ThreadsafeQueue<T>> queue_;
    sem_t *semphore_;
};

template <typename T>
class Subscriber {
   public:
    Subscriber(std::shared_ptr<ThreadsafeQueue<T>> queue, sem_t *sem)
        : queue_(queue), semphore_(sem)
    {
    }

   protected:
    std::shared_ptr<ThreadsafeQueue<T>> queue_;
    sem_t *semphore_;
};

#endif /* __NOYIFY_HH__ */
