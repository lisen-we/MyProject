#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <utility>
#include <vector>

// Thread-safe bounded producer-consumer queue. Compare determines which item
// has the highest priority, using the same rules as std::priority_queue.
template <typename T, typename Compare = std::less<T>>
class PriorityPCQueue {
public:
    explicit PriorityPCQueue(
        std::size_t capacity,
        Compare compare = Compare{})
        : queue_(compare), capacity_(capacity) {
        if (capacity_ == 0) {
            throw std::invalid_argument(
                "PriorityPCQueue capacity must be greater than zero.");
        }
    }

    PriorityPCQueue(const PriorityPCQueue&) = delete;
    PriorityPCQueue& operator=(const PriorityPCQueue&) = delete;
    PriorityPCQueue(PriorityPCQueue&&) = delete;
    PriorityPCQueue& operator=(PriorityPCQueue&&) = delete;

    bool push(T item) {
        std::unique_lock<std::mutex> lock(mutex_);
        producer_cv_.wait(lock, [this] {
            return shutdown_ || queue_.size() < capacity_;
        });

        if (shutdown_) {
            return false;
        }

        queue_.push(std::move(item));
        lock.unlock();
        consumer_cv_.notify_one();
        return true;
    }

    bool pop(T& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        consumer_cv_.wait(lock, [this] {
            return shutdown_ || !queue_.empty();
        });

        if (queue_.empty()) {
            return false;
        }

        item = queue_.top();
        queue_.pop();
        lock.unlock();
        producer_cv_.notify_one();
        return true;
    }

    void shut_down() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            shutdown_ = true;
        }

        producer_cv_.notify_all();
        consumer_cv_.notify_all();
    }

    bool is_shutdown() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return shutdown_;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

private:
    mutable std::mutex mutex_;
    std::priority_queue<T, std::vector<T>, Compare> queue_;
    const std::size_t capacity_;
    bool shutdown_ = false;
    std::condition_variable producer_cv_;
    std::condition_variable consumer_cv_;
};
