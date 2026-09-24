#include "market/book.hpp"
#include "market/queue.hpp"
#include "market/replay.hpp"
#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <vector>

#define CHECK(expr) do { if (!(expr)) throw std::runtime_error("check failed: " #expr); } while (false)
using namespace std::chrono_literals;
using namespace market;

void parser_tests() {
    CHECK(parse("1,ABC,R,0,-,0,0"));
    CHECK(parse("2,ABC,A,1,B,100,10\r"));
    for (auto line : {"", "0,A,R,0,-,0,0", "1,a,R,0,-,0,0", "1,A,R,0,-,0,0,",
                      "1,A,A,1,B,100,0", "1,A,A,1,B,100,1000000001", "1,A,X,1,B,0,0",
                      "1,A,E,1,-,1,2", "+1,A,R,0,-,0,0", "1,A,R,0,-,0,0x",
                      "18446744073709551616,A,R,0,-,0,0", "1,A,Z,0,-,0,0"}) CHECK(!parse(line));
}
void book_tests() {
    Book b;
    auto apply = [&](std::string_view line) { auto e = parse(line); CHECK(e); b.apply(*e); };
    apply("1,A,R,0,-,0,0");
    apply("2,A,A,10,B,100,10");
    apply("3,A,A,11,B,100,20");
    CHECK(b.bids().at(100) == 30);
    apply("4,A,A,10,S,101,5"); // duplicate id: atomic rejection, sequence consumed
    CHECK(b.asks().empty());
    apply("5,A,E,10,-,0,11"); // overfill
    CHECK(b.bids().at(100) == 30);
    apply("6,A,E,10,-,0,4");
    CHECK(b.orders().at(10).quantity == 6);
    apply("7,A,X,11,-,0,0");
    apply("8,A,E,10,-,0,6");
    CHECK(b.orders().empty() && b.bids().empty());
    apply("8,A,A,20,B,100,5"); // stale
    apply("10,A,A,20,B,100,5"); // gap
    CHECK(!b.synced() && b.orders().empty());
    apply("11,A,A,20,B,100,5"); // ignored
    apply("20,A,R,0,-,0,0"); // authoritative reset
    apply("21,A,A,20,S,101,5");
    CHECK(b.synced() && b.asks().at(101) == 5);
    CHECK(b.counters().rejected == 2 && b.counters().stale == 1);
    CHECK(b.counters().gaps == 1 && b.counters().ignored == 2);
}
void queue_tests() {
    bool threw = false;
    try { BoundedQueue<int> bad(0); } catch (const std::invalid_argument&) { threw = true; }
    CHECK(threw);
    BoundedQueue<int> q(1);
    CHECK(q.push(1));
    auto blocked = std::async(std::launch::async, [&] { return q.push(2); });
    // Wait for the observable blocked condition (not a sleep-based scheduling guess).
    auto deadline = std::chrono::steady_clock::now() + 2s;
    while (!q.stats().blocked_pushes && std::chrono::steady_clock::now() < deadline) std::this_thread::yield();
    const bool observed = q.stats().blocked_pushes == 1;
    q.close();
    CHECK(observed);
    CHECK(!blocked.get());
    CHECK(q.pop() == 1 && !q.pop());
    CHECK(q.stats().high_water == 1);
    q.close();
    BoundedQueue<int> empty(1);
    auto reader = std::async(std::launch::async, [&] { return empty.pop(); });
    empty.close();
    CHECK(!reader.get());
    BoundedQueue<int> many(7);
    std::atomic<long long> sum{};
    std::vector<std::jthread> consumers, producers;
    for (int c = 0; c < 3; ++c) consumers.emplace_back([&] { while (auto x = many.pop()) sum += *x; });
    for (int p = 0; p < 4; ++p) producers.emplace_back([&] { for (int i = 1; i <= 10000; ++i) CHECK(many.push(i)); });
    for (auto& t : producers) t.join();
    many.close();
    for (auto& t : consumers) t.join();
    CHECK(sum == 4LL * 10000 * 10001 / 2);
    CHECK(many.stats().high_water <= 7);
}
void replay_tests() {
    std::string data = "1,A,R,0,-,0,0\n2,A,A,1,B,10,4\n" + std::string(5000, 'x') + "\n3,A,E,1,-,0,2";
    std::string expected;
    for (std::size_t workers : {1U, 2U, 4U}) {
        std::istringstream in(data);
        auto result = replay(in, {workers, 1});
        std::ostringstream out;
        result.snapshot(out);
        CHECK(result.malformed == 1 && result.events == 3 && result.latency.count == 3);
        if (expected.empty()) expected = out.str();
        CHECK(expected == out.str());
    }
    std::istringstream broken;
    broken.setstate(std::ios::badbit);
    bool threw = false;
    try { (void)replay(broken, {4, 1}); } catch (const std::runtime_error&) { threw = true; }
    CHECK(threw);
    Histogram h;
    h.record(0); h.record(1); h.record(8);
    CHECK(h.percentile(50) == 1 && h.percentile(99) == 15);
}
int main() {
    try { parser_tests(); book_tests(); queue_tests(); replay_tests(); std::cout << "all unit tests passed\n"; }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
