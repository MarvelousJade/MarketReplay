#include "market/replay.hpp"
#include "market/queue.hpp"
#include <algorithm>
#include <bit>
#include <exception>
#include <istream>
#include <limits>
#include <memory>
#include <mutex>
#include <ostream>
#include <thread>
#include <vector>

namespace market {
void Histogram::record(Number ns) {
    const auto bucket = std::min<unsigned>(63, static_cast<unsigned>(std::bit_width(ns)));
    ++buckets[bucket]; ++count;
}
Number Histogram::percentile(unsigned numerator, unsigned denominator) const {
    if (!numerator || !denominator || numerator > denominator)
        throw std::invalid_argument("percentile must be in (0, 1]");
    if (!count) return 0;
    // Split before multiplication to avoid overflowing large sample counts.
    const auto rank = (count / denominator) * numerator
        + ((count % denominator) * numerator + denominator - 1) / denominator;
    Number sum = 0;
    for (unsigned i = 0; i < 64; ++i) {
        sum += buckets[i];
        if (sum >= rank) return i == 63 ? std::numeric_limits<Number>::max() : (Number{1} << i) - 1;
    }
    return 0;
}
Histogram& Histogram::operator+=(const Histogram& other) {
    for (std::size_t i = 0; i < 64; ++i) buckets[i] += other.buckets[i];
    count += other.count; return *this;
}
namespace {
using Clock = std::chrono::steady_clock;
struct Work { Event event; Clock::time_point entered; };
struct Worker {
    explicit Worker(std::size_t capacity) : queue(capacity) {}
    BoundedQueue<Work> queue;
    std::map<std::string, Book> books;
    Histogram latency;
};
std::size_t shard(std::string_view symbol, std::size_t workers) {
    Number hash = 14695981039346656037ULL;
    for (unsigned char c : symbol) { hash ^= c; hash *= 1099511628211ULL; }
    return static_cast<std::size_t>(hash % workers);
}
}
Result replay(std::istream& input, Config config) {
    if (!config.workers || config.workers > 256 || !config.capacity)
        throw std::invalid_argument("workers must be 1..256; capacity must be positive");
    // Reject prior I/O failures before getline recovery can clear their evidence.
    if (input.fail()) throw std::runtime_error("input stream is not readable");
    Result result;
    std::vector<std::unique_ptr<Worker>> workers;
    for (std::size_t i = 0; i < config.workers; ++i)
        workers.push_back(std::make_unique<Worker>(config.capacity));
    std::mutex error_mutex;
    std::exception_ptr error;
    auto close = [&] { for (auto& w : workers) w->queue.close(); };
    auto fail = [&] {
        { std::lock_guard lock(error_mutex); if (!error) error = std::current_exception(); }
        close();
    };
    // Declared after queues/error state: jthreads cannot outlive their dependencies.
    std::vector<std::jthread> threads;
    const auto start = Clock::now();
    try {
        for (auto& ptr : workers) {
            threads.emplace_back([&, w = ptr.get()] {
                try {
                    while (auto work = w->queue.pop()) {
                        w->books[work->event.symbol].apply(work->event);
                        const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - work->entered).count();
                        w->latency.record(static_cast<Number>(ns));
                    }
                } catch (...) { fail(); }
            });
        }
        // Fixed storage bounds malformed-line memory; long records are discarded whole.
        char line[4098];
        while (true) {
            input.getline(line, sizeof(line));
            if (input.bad()) throw std::runtime_error("input read failed");
            if (input.fail() && !input.eof()) {
                input.clear(); input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                if (input.bad()) throw std::runtime_error("input read failed");
                ++result.malformed;
                continue;
            }
            auto length = input.gcount();
            if (!length && input.eof()) break;
            if (!input.eof()) --length; // getline counts the delimiter, not stored.
            if (length > 4096) { ++result.malformed; continue; }
            auto event = parse(std::string_view(line, static_cast<std::size_t>(length)));
            if (!event) { ++result.malformed; continue; }
            auto& target = workers[shard(event->symbol, config.workers)];
            if (!target->queue.push(Work{std::move(*event), Clock::now()})) break;
            ++result.events;
        }
    } catch (...) { fail(); }
    close();
    for (auto& thread : threads) thread.join();
    if (error) std::rethrow_exception(error);
    result.seconds = std::chrono::duration<double>(Clock::now() - start).count();
    for (auto& w : workers) {
        result.books.merge(w->books);
        result.latency += w->latency;
        const auto stats = w->queue.stats();
        result.high_water = std::max(result.high_water, stats.high_water);
        result.blocked_pushes += stats.blocked_pushes;
    }
    return result;
}
void Result::snapshot(std::ostream& out) const {
    Counters total;
    for (const auto& [symbol, book] : books) {
        total += book.counters();
        out << "S," << symbol << ',' << book.sequence() << ',' << book.synced() << '\n';
        for (const auto& [id, o] : book.orders())
            out << "O," << symbol << ',' << id << ',' << o.side << ',' << o.price << ',' << o.quantity << '\n';
        for (auto it = book.bids().rbegin(); it != book.bids().rend(); ++it)
            out << "L," << symbol << ",B," << it->first << ',' << it->second << '\n';
        for (const auto& [price, quantity] : book.asks())
            out << "L," << symbol << ",S," << price << ',' << quantity << '\n';
    }
    out << "C," << malformed << ',' << total.applied << ',' << total.rejected << ','
        << total.stale << ',' << total.gaps << ',' << total.ignored << '\n';
}
void Result::metrics(std::ostream& out, Config config) const {
    out << "{\"events\":" << events << ",\"malformed\":" << malformed
        << ",\"workers\":" << config.workers << ",\"capacity\":" << config.capacity
        << ",\"seconds\":" << seconds << ",\"events_per_second\":" << (seconds > 0 ? static_cast<double>(events) / seconds : 0)
        << ",\"latency_samples\":" << latency.count
        << ",\"latency_p50_upper_ns\":" << latency.percentile(50)
        << ",\"latency_p99_upper_ns\":" << latency.percentile(99)
        << ",\"latency_p999_upper_ns\":" << latency.percentile(999, 1000)
        << ",\"queue_high_water\":" << high_water
        << ",\"blocked_pushes\":" << blocked_pushes << "}\n";
}
}
