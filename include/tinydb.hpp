#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iomanip>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace tinydb {

class error : public std::runtime_error {
public:
    explicit error(const std::string& message) : std::runtime_error(message) {}
};

using value = std::variant<std::nullptr_t, bool, std::int64_t, double, std::string>;
using row = std::unordered_map<std::string, value>;

namespace detail {

inline std::string trim(std::string text) {
    auto not_space = [](unsigned char c) { return !std::isspace(c); };
    text.erase(text.begin(), std::find_if(text.begin(), text.end(), not_space));
    text.erase(std::find_if(text.rbegin(), text.rend(), not_space).base(), text.end());
    return text;
}

inline std::string to_lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return text;
}

inline bool is_numeric(const value& v) {
    return std::holds_alternative<std::int64_t>(v) || std::holds_alternative<double>(v);
}

inline double to_double(const value& v) {
    if (std::holds_alternative<std::int64_t>(v)) {
        return static_cast<double>(std::get<std::int64_t>(v));
    }
    if (std::holds_alternative<double>(v)) {
        return std::get<double>(v);
    }
    throw error("value is not numeric");
}

inline std::string to_string(const value& v) {
    if (std::holds_alternative<std::nullptr_t>(v)) {
        return "null";
    }
    if (std::holds_alternative<bool>(v)) {
        return std::get<bool>(v) ? "true" : "false";
    }
    if (std::holds_alternative<std::int64_t>(v)) {
        return std::to_string(std::get<std::int64_t>(v));
    }
    if (std::holds_alternative<double>(v)) {
        std::ostringstream stream;
        stream << std::fixed << std::setprecision(6) << std::get<double>(v);
        auto result = stream.str();
        result.erase(result.find_last_not_of('0') + 1);
        if (!result.empty() && result.back() == '.') {
            result.pop_back();
        }
        return result;
    }
    return std::get<std::string>(v);
}

inline bool values_equal(const value& lhs, const value& rhs) {
    if (is_numeric(lhs) && is_numeric(rhs)) {
        return std::fabs(to_double(lhs) - to_double(rhs)) < 1e-9;
    }
    if (lhs.index() != rhs.index()) {
        return false;
    }
    return lhs == rhs;
}

inline int compare_values(const value& lhs, const value& rhs) {
    if (is_numeric(lhs) && is_numeric(rhs)) {
        const auto left = to_double(lhs);
        const auto right = to_double(rhs);
        if (std::fabs(left - right) < 1e-9) {
            return 0;
        }
        return left < right ? -1 : 1;
    }

    if (std::holds_alternative<std::string>(lhs) && std::holds_alternative<std::string>(rhs)) {
        const auto& left = std::get<std::string>(lhs);
        const auto& right = std::get<std::string>(rhs);
        if (left == right) {
            return 0;
        }
        return left < right ? -1 : 1;
    }

    if (std::holds_alternative<bool>(lhs) && std::holds_alternative<bool>(rhs)) {
        const auto left = std::get<bool>(lhs);
        const auto right = std::get<bool>(rhs);
        if (left == right) {
            return 0;
        }
        return left ? 1 : -1;
    }

    throw error("comparison is only supported for matching scalar types");
}

inline value parse_literal(std::string text) {
    text = trim(text);
    if (text.empty()) {
        throw error("empty literal");
    }

    const auto lower = to_lower(text);
    if (lower == "null") {
        return nullptr;
    }
    if (lower == "true") {
        return true;
    }
    if (lower == "false") {
        return false;
    }

    if ((text.front() == '\'' && text.back() == '\'') || (text.front() == '"' && text.back() == '"')) {
        return text.substr(1, text.size() - 2);
    }

    try {
        std::size_t pos = 0;
        const auto v = std::stoll(text, &pos);
        if (pos == text.size()) {
            return static_cast<std::int64_t>(v);
        }
    } catch (...) {
    }

    try {
        std::size_t pos = 0;
        const auto v = std::stod(text, &pos);
        if (pos == text.size()) {
            return v;
        }
    } catch (...) {
    }

    return text;
}

} // namespace detail

class result_set {
public:
    result_set() = default;
    explicit result_set(std::vector<row> rows) : rows_(std::move(rows)) {}

    [[nodiscard]] const std::vector<row>& rows() const noexcept { return rows_; }
    [[nodiscard]] std::size_t size() const noexcept { return rows_.size(); }
    [[nodiscard]] bool empty() const noexcept { return rows_.empty(); }

    const row& operator[](std::size_t index) const { return rows_.at(index); }

private:
    std::vector<row> rows_;
};

class table {
public:
    table() = default;
    explicit table(std::vector<std::string> columns) : columns_(std::move(columns)) {
        if (columns_.empty()) {
            throw error("table must contain at least one column");
        }
    }

    [[nodiscard]] const std::vector<std::string>& columns() const noexcept { return columns_; }
    [[nodiscard]] const std::vector<row>& rows() const noexcept { return rows_; }
    [[nodiscard]] std::size_t size() const noexcept { return rows_.size(); }

    table& insert(row data) {
        validate_columns(data);
        rows_.push_back(std::move(data));
        return *this;
    }

    table& insert(std::initializer_list<std::pair<const std::string, value>> values) {
        row data;
        for (const auto& [key, val] : values) {
            data.emplace(key, val);
        }
        return insert(std::move(data));
    }

    result_set select(const std::vector<std::string>& selected_columns) const {
        return filter([&](const row&) { return true; }, selected_columns);
    }

    result_set where(const std::function<bool(const row&)>& predicate) const {
        return filter(predicate, columns_);
    }

    result_set where(const std::string& column, const std::function<bool(const value&)>& predicate) const {
        return filter([
            &](const row& current) {
                const auto it = current.find(column);
                return it != current.end() && predicate(it->second);
            },
            columns_);
    }

    result_set eq(const std::string& column, const value& expected) const {
        return where(column, [&](const value& current) {
            return detail::values_equal(current, expected);
        });
    }

    result_set gt(const std::string& column, const value& expected) const {
        return where(column, [&](const value& current) {
            return detail::compare_values(current, expected) > 0;
        });
    }

    result_set lt(const std::string& column, const value& expected) const {
        return where(column, [&](const value& current) {
            return detail::compare_values(current, expected) < 0;
        });
    }

    [[nodiscard]] bool has_column(const std::string& name) const {
        return std::find(columns_.begin(), columns_.end(), name) != columns_.end();
    }

private:
    std::vector<std::string> columns_;
    std::vector<row> rows_;

    void validate_columns(const row& data) const {
        for (const auto& [column, _] : data) {
            if (!has_column(column)) {
                throw error("unknown column: " + column);
            }
        }
    }

    result_set filter(const std::function<bool(const row&)>& predicate,
                      const std::vector<std::string>& selected_columns) const {
        for (const auto& column : selected_columns) {
            if (!has_column(column)) {
                throw error("unknown column in select: " + column);
            }
        }

        std::vector<row> output;
        for (const auto& current : rows_) {
            if (!predicate(current)) {
                continue;
            }

            row projected;
            for (const auto& column : selected_columns) {
                const auto it = current.find(column);
                if (it != current.end()) {
                    projected.emplace(column, it->second);
                } else {
                    projected.emplace(column, nullptr);
                }
            }
            output.push_back(std::move(projected));
        }
        return result_set(std::move(output));
    }
};

class database {
public:
    database() = default;

    table& create_table(const std::string& name, std::vector<std::string> columns) {
        if (tables_.find(name) != tables_.end()) {
            throw error("table already exists: " + name);
        }
        auto [it, _] = tables_.emplace(name, table(std::move(columns)));
        return it->second;
    }

    [[nodiscard]] bool has_table(const std::string& name) const {
        return tables_.find(name) != tables_.end();
    }

    table& get_table(const std::string& name) {
        auto it = tables_.find(name);
        if (it == tables_.end()) {
            throw error("unknown table: " + name);
        }
        return it->second;
    }

    const table& get_table(const std::string& name) const {
        auto it = tables_.find(name);
        if (it == tables_.end()) {
            throw error("unknown table: " + name);
        }
        return it->second;
    }

private:
    std::unordered_map<std::string, table> tables_;
};

struct query_result {
    std::size_t affected_rows = 0;
    result_set rows;
};

class engine {
public:
    engine() = default;

    database& db() noexcept { return db_; }
    const database& db() const noexcept { return db_; }

    query_result execute(const std::string& sql) {
        const auto normalized = detail::trim(sql);
        const auto lower = detail::to_lower(normalized);

        if (lower.rfind("create table", 0) == 0) {
            return execute_create_table(normalized);
        }
        if (lower.rfind("insert into", 0) == 0) {
            return execute_insert(normalized);
        }
        if (lower.rfind("select", 0) == 0) {
            return execute_select(normalized);
        }

        throw error("unsupported query");
    }

private:
    database db_;

    query_result execute_create_table(const std::string& sql) {
        const auto open = sql.find('(');
        const auto close = sql.find_last_of(')');
        if (open == std::string::npos || close == std::string::npos || close <= open) {
            throw error("invalid CREATE TABLE statement");
        }

        const auto prefix = detail::trim(sql.substr(std::string("create table").size(), open - std::string("create table").size()));
        const auto inside = sql.substr(open + 1, close - open - 1);

        std::vector<std::string> columns;
        std::stringstream stream(inside);
        std::string item;
        while (std::getline(stream, item, ',')) {
            item = detail::trim(item);
            if (!item.empty()) {
                columns.push_back(item);
            }
        }

        db_.create_table(prefix, columns);
        return {};
    }

    query_result execute_insert(const std::string& sql) {
        const auto lower = detail::to_lower(sql);
        const auto values_pos = lower.find(" values ");
        if (values_pos == std::string::npos) {
            throw error("invalid INSERT statement");
        }

        const auto table_name = detail::trim(sql.substr(std::string("insert into").size(), values_pos - std::string("insert into").size()));
        const auto open = sql.find('(', values_pos);
        const auto close = sql.find_last_of(')');
        if (open == std::string::npos || close == std::string::npos || close <= open) {
            throw error("invalid INSERT values");
        }

        auto& tbl = db_.get_table(table_name);
        std::stringstream stream(sql.substr(open + 1, close - open - 1));
        std::string token;
        row new_row;
        std::size_t index = 0;

        while (std::getline(stream, token, ',')) {
            if (index >= tbl.columns().size()) {
                throw error("too many values for insert");
            }
            new_row[tbl.columns()[index]] = detail::parse_literal(token);
            ++index;
        }

        if (index != tbl.columns().size()) {
            throw error("value count does not match column count");
        }

        tbl.insert(std::move(new_row));
        query_result result;
        result.affected_rows = 1;
        return result;
    }

    query_result execute_select(const std::string& sql) {
        const auto lower = detail::to_lower(sql);
        const auto from_pos = lower.find(" from ");
        if (from_pos == std::string::npos) {
            throw error("invalid SELECT statement");
        }

        const auto selected = detail::trim(sql.substr(std::string("select").size(), from_pos - std::string("select").size()));
        const auto where_pos = lower.find(" where ", from_pos + 6);

        const std::string table_name = where_pos == std::string::npos
            ? detail::trim(sql.substr(from_pos + 6))
            : detail::trim(sql.substr(from_pos + 6, where_pos - (from_pos + 6)));

        const auto& tbl = db_.get_table(table_name);
        std::vector<std::string> selected_columns;

        if (selected == "*") {
            selected_columns = tbl.columns();
        } else {
            std::stringstream stream(selected);
            std::string item;
            while (std::getline(stream, item, ',')) {
                selected_columns.push_back(detail::trim(item));
            }
        }

        query_result result;
        if (where_pos == std::string::npos) {
            result.rows = tbl.select(selected_columns);
            return result;
        }

        const auto condition = detail::trim(sql.substr(where_pos + 7));
        static const std::vector<std::string> operators = {">=", "<=", "=", ">", "<"};
        std::string selected_operator;
        std::size_t op_pos = std::string::npos;

        for (const auto& op : operators) {
            op_pos = condition.find(op);
            if (op_pos != std::string::npos) {
                selected_operator = op;
                break;
            }
        }

        if (op_pos == std::string::npos) {
            throw error("unsupported WHERE clause");
        }

        const auto column = detail::trim(condition.substr(0, op_pos));
        const auto expected = detail::parse_literal(condition.substr(op_pos + selected_operator.size()));

        auto filtered = tbl.where(column, [&](const value& current) {
            if (selected_operator == "=") {
                return detail::values_equal(current, expected);
            }
            if (selected_operator == ">") {
                return detail::compare_values(current, expected) > 0;
            }
            if (selected_operator == "<") {
                return detail::compare_values(current, expected) < 0;
            }
            if (selected_operator == ">=") {
                return detail::compare_values(current, expected) >= 0;
            }
            if (selected_operator == "<=") {
                return detail::compare_values(current, expected) <= 0;
            }
            return false;
        });

        std::vector<row> projected;
        for (const auto& current : filtered.rows()) {
            row out;
            for (const auto& column_name : selected_columns) {
                auto it = current.find(column_name);
                out[column_name] = it != current.end() ? it->second : nullptr;
            }
            projected.push_back(std::move(out));
        }

        result.rows = result_set(std::move(projected));
        return result;
    }
};

inline std::string dump(const result_set& result) {
    std::ostringstream stream;
    for (const auto& current : result.rows()) {
        stream << "{";
        bool first = true;
        for (const auto& [key, val] : current) {
            if (!first) {
                stream << ", ";
            }
            first = false;
            stream << key << ": " << detail::to_string(val);
        }
        stream << "}\n";
    }
    return stream.str();
}

} // namespace tinydb
