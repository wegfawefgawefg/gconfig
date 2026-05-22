#include "gconfig/value.hpp"

#include <cmath>

namespace gconfig {

ValueType value_type(const Value& value) {
    if (std::holds_alternative<bool>(value))
        return ValueType::Bool;
    if (std::holds_alternative<int>(value))
        return ValueType::Int;
    if (std::holds_alternative<float>(value))
        return ValueType::Float;
    if (std::holds_alternative<std::string>(value))
        return ValueType::String;
    return ValueType::Vec2;
}

const char* value_type_name(ValueType type) {
    switch (type) {
    case ValueType::Bool:
        return "bool";
    case ValueType::Int:
        return "int";
    case ValueType::Float:
        return "float";
    case ValueType::String:
        return "string";
    case ValueType::Vec2:
        return "vec2";
    }
    return "unknown";
}

const char* value_type_name(const Value& value) {
    return value_type_name(value_type(value));
}

bool same_value_type(const Value& a, const Value& b) {
    return value_type(a) == value_type(b);
}

std::optional<bool> as_bool(const Value& value) {
    if (const bool* ptr = std::get_if<bool>(&value))
        return *ptr;
    return std::nullopt;
}

std::optional<int> as_int(const Value& value) {
    if (const int* ptr = std::get_if<int>(&value))
        return *ptr;
    return std::nullopt;
}

std::optional<float> as_float(const Value& value) {
    if (const float* ptr = std::get_if<float>(&value))
        return *ptr;
    if (const int* ptr = std::get_if<int>(&value))
        return static_cast<float>(*ptr);
    return std::nullopt;
}

std::optional<std::string> as_string(const Value& value) {
    if (const std::string* ptr = std::get_if<std::string>(&value))
        return *ptr;
    return std::nullopt;
}

std::optional<Vec2> as_vec2(const Value& value) {
    if (const Vec2* ptr = std::get_if<Vec2>(&value))
        return *ptr;
    return std::nullopt;
}

bool value_equals(const Value& a, const Value& b) {
    if (!same_value_type(a, b))
        return false;
    if (const Vec2* av = std::get_if<Vec2>(&a)) {
        const Vec2* bv = std::get_if<Vec2>(&b);
        return bv && av->x == bv->x && av->y == bv->y;
    }
    return a == b;
}

} // namespace gconfig
