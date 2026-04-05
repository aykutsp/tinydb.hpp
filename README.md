# tinydb.hpp

A small, header-only, in-memory database for C++17.

`tinydb.hpp` gives you tables, rows, and a tiny subset of SQL in a single header
file. It is meant for situations where you need structured, query-able data
inside a binary but pulling in SQLite (or anything larger) would be overkill:
CLI tools, test fixtures, local data pipelines, quick prototypes, and embedded
utilities.

It is intentionally small. The whole thing is one header, no external
dependencies, no build-time code generation, no macros to configure.

## Features

- Single header, drop it in and `#include "tinydb.hpp"`
- No dependencies beyond the C++17 standard library
- Typed values via `std::variant` (null, bool, int64, double, string)
- Direct C++ API (`tinydb::table`) for full control
- A minimal SQL dialect (`CREATE TABLE`, `INSERT INTO`, `SELECT ... WHERE`)
- Header-only CMake target (`tinydb::tinydb`)

## Quick start

```cpp
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
}
```

## Typed table API

If you do not need SQL parsing, you can use the table API directly. It is
usually faster and avoids stringly-typed queries.

```cpp
tinydb::table users({"id", "name", "active"});

users.insert({
    {"id",     std::int64_t{1}},
    {"name",   std::string("Alice")},
    {"active", true},
});
users.insert({
    {"id",     std::int64_t{2}},
    {"name",   std::string("Bob")},
    {"active", false},
});

auto active = users.where("active", [](const tinydb::value& v) {
    return std::holds_alternative<bool>(v) && std::get<bool>(v);
});

auto adults = users.gt("id", std::int64_t{0});
```

## Supported SQL

```sql
CREATE TABLE users (id, name, age)
INSERT INTO users VALUES (1, 'Alice', 30)

SELECT * FROM users
SELECT id, name FROM users WHERE age > 25
SELECT id, city FROM users WHERE city = 'Berlin'
```

`WHERE` supports `=`, `>`, `<`, `>=`, and `<=`. String literals use single or
double quotes. Numeric literals are parsed as `int64` when possible, otherwise
`double`.

This is deliberately a small subset. If you need joins, grouping, indices, or
transactions, use a real database.

## Build

The project builds with any C++17 compiler and CMake 3.16+.

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

This produces three targets:

- `tinydb_example`   — the snippet from the Quick start section
- `tinydb_benchmark` — a small insert/scan benchmark
- `tinydb_tests`     — the test binary used by `ctest`

Examples and tests can be turned off with `-DTINYDB_BUILD_EXAMPLES=OFF` and
`-DTINYDB_BUILD_TESTS=OFF`.

## Using it in your own project

Because the library is header-only, the easiest integration is to copy
`include/tinydb.hpp` into your project.

With CMake you can also consume it via `add_subdirectory`:

```cmake
add_subdirectory(third_party/tinydb.hpp)
target_link_libraries(my_app PRIVATE tinydb::tinydb)
```

## Project layout

```
include/tinydb.hpp       header-only library
examples/basic.cpp       SQL-style usage
examples/benchmark.cpp   simple insert/scan benchmark
tests/test_basic.cpp     tests used by ctest
CMakeLists.txt
```

## License

MIT. See [LICENSE](LICENSE).
