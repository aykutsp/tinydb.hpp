#include <iostream>
#include "tinydb.hpp"

int main() {
    tinydb::engine db;

    db.execute("CREATE TABLE users (id, name, age, city)");
    db.execute("INSERT INTO users VALUES (1, 'Alice', 30, 'Berlin')");
    db.execute("INSERT INTO users VALUES (2, 'Bob', 24, 'Tallinn')");
    db.execute("INSERT INTO users VALUES (3, 'Carol', 35, 'Bursa')");

    auto result = db.execute("SELECT id, name, city FROM users WHERE age > 25");

    std::cout << tinydb::dump(result.rows);
    return 0;
}
