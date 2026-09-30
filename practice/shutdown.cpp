#include "market/queue.hpp"
#include <iostream>

int main() {
    market::BoundedQueue<int> queue(2);
    const bool accepted = queue.push(10) && queue.push(20);
    queue.close();
    int count = 0, sum = 0;
    while (auto item = queue.pop()) { ++count; sum += *item; }
    const bool late_push = queue.push(30);
    std::cout << "accepted=" << accepted << " delivered=" << count
              << " sum=" << sum << " late_push=" << late_push << '\n';
}
