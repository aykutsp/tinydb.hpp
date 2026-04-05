#include <cassert>
#include <iostream>
#include "tinydb.hpp"

static void can_insert_and_select_rows() {
    tinydb::engine db;
    db.execute("CREATE TABLE metrics (id, sensor, value)");
    db.execute("INSERT INTO metrics VALUES (1, 'temp', 21.5)");
    db.execute("INSERT INTO metrics VALUES (2, 'temp', 25.0)");
    db.execute("INSERT INTO metrics VALUES (3, 'pressure', 11.2)");

    auto result = db.execute("SELECT id, sensor FROM metrics WHERE value > 22");
    assert(result.rows.size() == 1);
}

static void table_api_supports_predicates() {
    tinydb::table users({"id", "name", "active"});
    users.insert({{"id", std::int64_t{1}}, {"name", std::string("Alice")}, {"active", true}});
    users.insert({{"id", std::int64_t{2}}, {"name", std::string("Bob")}, {"active", false}});

    auto result = users.where("active", [](const tinydb::value& v) {
        return std::holds_alternative<bool>(v) && std::get<bool>(v);
    });

    assert(result.size() == 1);
}

int main() {
    can_insert_and_select_rows();
    table_api_supports_predicates();
    std::cout << "All tests passed.\n";
    return 0;
}
