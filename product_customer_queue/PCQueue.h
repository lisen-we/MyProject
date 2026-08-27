#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <utility>

// 线程安全的有界生产者-消费者队列。
// shut_down() 后不再接受新元素，但允许消费者取完已有元素。
template <typename T>
class PCQueue {
public:
    explicit PCQueue(std::size_t capacity)
        : capacity_(capacity) {
        if (capacity_ == 0) {
            throw std::invalid_argument("PCQueue capacity must be greater than zero.");
        }
    }

    PCQueue(const PCQueue&) = delete;
    PCQueue& operator=(const PCQueue&) = delete;
    PCQueue(PCQueue&&) = delete;
    PCQueue& operator=(PCQueue&&) = delete;

    // 阻塞插入。队列关闭时返回 false。
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

    // 立即尝试插入。队列已满或已关闭时返回 false。
    bool try_push(T item) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (shutdown_ || queue_.size() >= capacity_) {
            return false;
        }

        queue_.push(std::move(item));
        lock.unlock();
        consumer_cv_.notify_one();
        return true;
    }

    // 在 timeout 时间内等待插入机会。
    template <typename Rep, typename Period>
    bool wait_push(
        T item,
        const std::chrono::duration<Rep, Period>& timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        const bool ready = producer_cv_.wait_for(lock, timeout, [this] {
            return shutdown_ || queue_.size() < capacity_;
        });

        if (!ready || shutdown_) {
            return false;
        }

        queue_.push(std::move(item));
        lock.unlock();
        consumer_cv_.notify_one();
        return true;
    }

    // 阻塞弹出。关闭且排空后返回 false。
    bool pop(T& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        consumer_cv_.wait(lock, [this] {
            return shutdown_ || !queue_.empty();
        });

        if (queue_.empty()) {
            return false;
        }

        item = std::move(queue_.front());
        queue_.pop();
        lock.unlock();
        producer_cv_.notify_one();
        return true;
    }

    // 立即尝试弹出。队列为空时返回 false。
    bool try_pop(T& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return false;
        }

        item = std::move(queue_.front());
        queue_.pop();
        lock.unlock();
        producer_cv_.notify_one();
        return true;
    }

    // 在 timeout 时间内等待元素。
    template <typename Rep, typename Period>
    bool wait_pop(
        T& item,
        const std::chrono::duration<Rep, Period>& timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        const bool ready = consumer_cv_.wait_for(lock, timeout, [this] {
            return shutdown_ || !queue_.empty();
        });

        if (!ready || queue_.empty()) {
            return false;
        }

        item = std::move(queue_.front());
        queue_.pop();
        lock.unlock();
        producer_cv_.notify_one();
        return true;
    }

    // 关闭队列，并唤醒所有等待中的生产者和消费者。
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

    std::size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    std::size_t capacity() const noexcept {
        return capacity_;
    }

private:
    mutable std::mutex mutex_;
    std::queue<T> queue_;
    const std::size_t capacity_;
    bool shutdown_ = false;
    std::condition_variable producer_cv_;
    std::condition_variable consumer_cv_;
};
