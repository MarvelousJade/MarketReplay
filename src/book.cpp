#include "market/book.hpp"
#include <limits>

namespace market {
Counters& Counters::operator+=(const Counters& r) {
    applied += r.applied; rejected += r.rejected; stale += r.stale;
    gaps += r.gaps; ignored += r.ignored;
    return *this;
}
void Book::apply(const Event& e) {
    if (e.sequence <= sequence_) { ++counters_.stale; return; }
    if (e.operation == 'R') {
        orders_.clear(); bids_.clear(); asks_.clear();
        sequence_ = e.sequence; synced_ = true; ++counters_.applied; return;
    }
    if (synced_ && e.sequence != sequence_ + 1) {
        synced_ = false; ++counters_.gaps;
    }
    sequence_ = e.sequence;
    if (!synced_) { ++counters_.ignored; return; }
    auto it = orders_.find(e.id);
    if (e.operation == 'A') {
        if (it != orders_.end()) { ++counters_.rejected; return; }
        auto& levels = e.side == 'B' ? bids_ : asks_;
        const auto level = levels.find(e.price);
        const Number total = level == levels.end() ? 0 : level->second;
        if (e.quantity > std::numeric_limits<Number>::max() - total) {
            ++counters_.rejected; return;
        }
        orders_.emplace(e.id, Order{e.side, e.price, e.quantity});
        levels[e.price] = total + e.quantity;
    } else {
        if (it == orders_.end() || (e.operation == 'E' && e.quantity > it->second.quantity)) {
            ++counters_.rejected; return;
        }
        auto& order = it->second;
        auto& levels = order.side == 'B' ? bids_ : asks_;
        const auto amount = e.operation == 'X' ? order.quantity : e.quantity;
        auto level = levels.find(order.price);
        level->second -= amount;
        if (!level->second) levels.erase(level);
        order.quantity -= amount;
        if (!order.quantity) orders_.erase(it);
    }
    ++counters_.applied;
}
}
