#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>
#include <utility>

// 线程安全的有界阻塞队列。
// shutdown() 后停止接收新元素，但消费者仍可取完已有元素。
// 销毁队列前，调用者必须确保所有访问队列的线程已经退出并 join。
template <typename T>
class BlockingQueue {
public:
    explicit BlockingQueue(std::size_t capacity)
        : capacity_(capacity) {
        if (capacity_ == 0) {
            throw std::invalid_argument("BlockingQueue capacity must be greater than zero.");
        }
    }

    BlockingQueue(const BlockingQueue&) = delete;
    BlockingQueue& operator=(const BlockingQueue&) = delete;
    BlockingQueue(BlockingQueue&&) = delete;
    BlockingQueue& operator=(BlockingQueue&&) = delete;

    // 阻塞直到有空位或队列关闭。
    bool push(T item) {
        std::unique_lock<std::mutex> lock(mutex_);
        producer_cv_.wait(lock, [this] {
            return queue_.size() < capacity_ || shutdown_;
        });

        if (shutdown_) {
            return false;
        }

        queue_.push(std::move(item));
        lock.unlock();
        consumer_cv_.notify_one();
        return true;
    }

    // 立即尝试入队；队列已满或已关闭时返回 false。
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

    // 在指定时间内等待空位；超时或队列关闭时返回 false。
    template <typename Rep, typename Period>
    bool wait_push_for(T item, const std::chrono::duration<Rep, Period>& timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        const bool ready = producer_cv_.wait_for(lock, timeout, [this] {
            return queue_.size() < capacity_ || shutdown_;
        });

        if (!ready || shutdown_) {
            return false;
        }

        queue_.push(std::move(item));
        lock.unlock();
        consumer_cv_.notify_one();
        return true;
    }

    // 阻塞直到有元素，或队列关闭并排空。
    // nullopt 表示队列已经关闭且没有剩余元素。
    std::optional<T> pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        consumer_cv_.wait(lock, [this] {
            return !queue_.empty() || shutdown_;
        });

        if (queue_.empty()) {
            return std::nullopt;
        }

        T item = std::move(queue_.front());
        queue_.pop();
        lock.unlock();
        producer_cv_.notify_one();
        return item;
    }

    // 立即尝试出队；关闭后仍会取出队列中的剩余元素。
    std::optional<T> try_pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return std::nullopt;
        }

        T item = std::move(queue_.front());
        queue_.pop();
        lock.unlock();
        producer_cv_.notify_one();
        return item;
    }

    // 在指定时间内等待元素。nullopt 表示超时或队列关闭且排空。
    template <typename Rep, typename Period>
    std::optional<T> wait_pop_for(
        const std::chrono::duration<Rep, Period>& timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        const bool ready = consumer_cv_.wait_for(lock, timeout, [this] {
            return !queue_.empty() || shutdown_;
        });

        if (!ready || queue_.empty()) {
            return std::nullopt;
        }


        T item = std::move(queue_.front());
        queue_.pop();
        lock.unlock();
        producer_cv_.notify_one();
        return item;
    }

    // 幂等关闭，可重复调用。
    void shutdown() {
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
    std::queue<T> queue_;
    const std::size_t capacity_;
    mutable std::mutex mutex_;
    std::condition_variable producer_cv_;
    std::condition_variable consumer_cv_;
    bool shutdown_ = false;
};
