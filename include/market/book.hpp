#pragma once
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace market {
using Number = std::uint64_t;
inline constexpr Number max_number = 1'000'000'000'000ULL;
inline constexpr Number max_quantity = 1'000'000'000ULL;
struct Event {
    Number sequence{}, id{}, price{}, quantity{};
    std::string symbol;
    char operation{}, side{};
};
std::optional<Event> parse(std::string_view line);
struct Order { char side; Number price, quantity; };
struct Counters {
    Number applied{}, rejected{}, stale{}, gaps{}, ignored{};
    Counters& operator+=(const Counters& rhs);
};
class Book {
public:
    void apply(const Event& event);
    Number sequence() const { return sequence_; }
    bool synced() const { return synced_; }
    const auto& orders() const { return orders_; }
    const auto& bids() const { return bids_; }
    const auto& asks() const { return asks_; }
    const Counters& counters() const { return counters_; }
private:
    Number sequence_{};
    bool synced_{};
    std::map<Number, Order> orders_;
    std::map<Number, Number> bids_, asks_;
    Counters counters_;
};
}
