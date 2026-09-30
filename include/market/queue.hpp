#pragma once
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <utility>

namespace market {
// Blocking MPMC queue. close() is idempotent; pending items drain before EOF.
// Owners must close and join users before destroying the queue.
template<class T> class BoundedQueue {
public:
    struct Stats { std::size_t high_water{}, blocked_pushes{}; };
    explicit BoundedQueue(std::size_t capacity) : capacity_(capacity) {
        if (!capacity) throw std::invalid_argument("queue capacity must be positive");
    }
    BoundedQueue(const BoundedQueue&) = delete;
    BoundedQueue& operator=(const BoundedQueue&) = delete;
    bool push(T value) {
        std::unique_lock lock(mutex_);
        if (!closed_ && items_.size() == capacity_) ++stats_.blocked_pushes;
        writable_.wait(lock, [&] { return closed_ || items_.size() < capacity_; });
        if (closed_) return false;
        items_.push_back(std::move(value));
        if (items_.size() > stats_.high_water) stats_.high_water = items_.size();
        lock.unlock();
        readable_.notify_one();
        return true;
    }
    std::optional<T> pop() {
        std::unique_lock lock(mutex_);
        readable_.wait(lock, [&] { return closed_ || !items_.empty(); });
        if (closed_ || items_.empty()) return std::nullopt;
        T item = std::move(items_.front());
        items_.pop_front();
        lock.unlock();
        writable_.notify_one();
        return item;
    }
    void close() {
        { std::lock_guard lock(mutex_); closed_ = true; }
        readable_.notify_all();
        writable_.notify_all();
    }
    Stats stats() const { std::lock_guard lock(mutex_); return stats_; }
private:
    const std::size_t capacity_;
    mutable std::mutex mutex_;
    std::condition_variable readable_, writable_;
    std::deque<T> items_;
    bool closed_{};
    Stats stats_;
};
}
