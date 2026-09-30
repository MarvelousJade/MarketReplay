#include "market/book.hpp"
#include <iostream>
#include <stdexcept>

using namespace market;
int main() {
    try {
        for (char side : {'B', 'S'}) {
            for (Number executed : {3ULL, 4ULL, 5ULL}) {
                Book book;
                book.apply(Event{1, 0, 0, 0, "A", 'R', '-'});
                book.apply(Event{2, 1, 100, 4, "A", 'A', side});
                book.apply(Event{3, 2, 100, 7, "A", 'A', side});
                book.apply(Event{4, 1, 0, executed, "A", 'E', '-'});
                const bool rejected = executed > 4;
                const Number remaining = rejected ? 4 : 4 - executed;
                const auto& levels = side == 'B' ? book.bids() : book.asks();
                if (book.sequence() != 4 || !book.synced()
                    || book.counters().rejected != static_cast<Number>(rejected)
                    || book.counters().applied != (rejected ? 3U : 4U)
                    || levels.at(100) != remaining + 7
                    || book.orders().contains(1) != (remaining != 0)
                    || (remaining && book.orders().at(1).quantity != remaining)
                    || book.orders().at(2).quantity != 7)
                    throw std::runtime_error("execution boundary failed: side=" + std::string(1, side)
                                             + " quantity=" + std::to_string(executed));
            }
        }
        std::cout << "below/equal/above executions preserve order and shared-level invariants\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
