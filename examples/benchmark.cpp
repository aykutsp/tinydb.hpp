#include <chrono>
#include <iostream>
#include <string>

#include "tinydb.hpp"

namespace {

using clock_type = std::chrono::steady_clock;

double ms_since(clock_type::time_point start) {
    const auto end = clock_type::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

} // namespace

int main() {
    constexpr int row_count = 50'000;

    tinydb::table events({"id", "kind", "value"});

    const auto insert_start = clock_type::now();
    for (int i = 0; i < row_count; ++i) {
        events.insert({
            {"id", static_cast<std::int64_t>(i)},
            {"kind", std::string(i % 2 == 0 ? "click" : "view")},
            {"value", static_cast<std::int64_t>(i % 1000)},
        });
    }
    const auto insert_ms = ms_since(insert_start);

    const auto scan_start = clock_type::now();
    const auto filtered = events.gt("value", static_cast<std::int64_t>(900));
    const auto scan_ms = ms_since(scan_start);

    const auto eq_start = clock_type::now();
    const auto clicks = events.eq("kind", std::string("click"));
    const auto eq_ms = ms_since(eq_start);

    std::cout << "rows inserted : " << row_count << " (" << insert_ms << " ms)\n";
    std::cout << "range scan    : " << filtered.size() << " rows (" << scan_ms << " ms)\n";
    std::cout << "equality scan : " << clicks.size() << " rows (" << eq_ms << " ms)\n";
    return 0;
}
