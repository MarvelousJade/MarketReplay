#pragma once
#include "market/book.hpp"
#include <array>
#include <chrono>
#include <cstddef>
#include <iosfwd>

namespace market {
struct Histogram {
    std::array<Number, 64> buckets{};
    Number count{};
    void record(Number nanoseconds);
    // Nearest-rank upper bound; denominator=1000 supports 99.9% as 999/1000.
    Number percentile(unsigned numerator, unsigned denominator = 100) const;
    Histogram& operator+=(const Histogram& other);
};
struct Config { std::size_t workers{2}, capacity{1024}; };
struct Result {
    std::map<std::string, Book> books;
    Number malformed{}, events{}, blocked_pushes{};
    std::size_t high_water{};
    double seconds{};
    Histogram latency;
    void snapshot(std::ostream& out) const;
    void metrics(std::ostream& out, Config config) const;
};
Result replay(std::istream& input, Config config);
}
