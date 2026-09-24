#include "market/replay.hpp"
#include <charconv>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string_view>

int main(int argc, char** argv) {
    try {
        if (argc < 2 || argc > 4) {
            std::cerr << "usage: replay INPUT.csv [workers=2] [queue_capacity=1024]\n";
            return 2;
        }
        auto size = [](std::string_view arg) {
            std::size_t value{};
            auto [end, ec] = std::from_chars(arg.data(), arg.data() + arg.size(), value);
            if (ec != std::errc{} || end != arg.data() + arg.size() || !value)
                throw std::invalid_argument("expected positive integer");
            return value;
        };
        market::Config config;
        if (argc >= 3) config.workers = size(argv[2]);
        if (argc >= 4) config.capacity = size(argv[3]);
        std::ifstream input(argv[1], std::ios::binary);
        if (!input) throw std::runtime_error("cannot open input file");
        auto result = market::replay(input, config);
        result.snapshot(std::cout);
        std::cout.flush();
        if (!std::cout) throw std::runtime_error("snapshot write failed");
        result.metrics(std::cerr, config);
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "replay: " << e.what() << '\n';
        return 1;
    }
}
