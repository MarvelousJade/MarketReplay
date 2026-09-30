#include "market/queue.hpp"
#include <iostream>
#include <stdexcept>

int main() {
    try {
        market::BoundedQueue<int> queue(3);
        for (int value : {10, 20, 30})
            if (!queue.push(value)) throw std::runtime_error("open queue refused work");
        queue.close();
        queue.close();
        if (queue.push(40)) throw std::runtime_error("closed queue accepted new work");
        for (int expected : {10, 20, 30})
            if (queue.pop() != expected)
                throw std::runtime_error("closed queue failed FIFO drain: expected " + std::to_string(expected));
        if (queue.pop() || queue.pop()) throw std::runtime_error("drained queue did not return EOF");
        if (queue.stats().high_water != 3) throw std::runtime_error("queue statistics changed during shutdown");
        std::cout << "closed queue drains accepted work FIFO and refuses new pushes\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
