#include "market/book.hpp"
#include <array>
#include <charconv>

namespace market {
std::optional<Event> parse(std::string_view line) {
    if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
    std::array<std::string_view, 7> fields;
    for (std::size_t i = 0; i < fields.size(); ++i) {
        const auto comma = line.find(',');
        if ((i == 6) != (comma == std::string_view::npos)) return std::nullopt;
        fields[i] = line.substr(0, comma);
        if (i != 6) line.remove_prefix(comma + 1);
    }
    auto number = [](std::string_view text, Number& out) {
        if (text.empty()) return false;
        for (char c : text) if (c < '0' || c > '9') return false;
        auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), out);
        return error == std::errc{} && end == text.data() + text.size() && out <= max_number;
    };
    Event e;
    if (!number(fields[0], e.sequence) || !e.sequence ||
        !number(fields[3], e.id) || !number(fields[5], e.price) ||
        !number(fields[6], e.quantity) || e.quantity > max_quantity) return std::nullopt;
    if (fields[1].empty() || fields[1].size() > 16) return std::nullopt;
    for (char c : fields[1])
        if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_')) return std::nullopt;
    if (fields[2].size() != 1 || fields[4].size() != 1) return std::nullopt;
    e.symbol = fields[1]; e.operation = fields[2][0]; e.side = fields[4][0];
    switch (e.operation) {
    case 'R': if (e.id || e.price || e.quantity || e.side != '-') return std::nullopt; break;
    case 'A': if (!e.id || !e.price || !e.quantity || (e.side != 'B' && e.side != 'S')) return std::nullopt; break;
    case 'X': if (!e.id || e.price || e.quantity || e.side != '-') return std::nullopt; break;
    case 'E': if (!e.id || e.price || !e.quantity || e.side != '-') return std::nullopt; break;
    default: return std::nullopt;
    }
    return e;
}
}
