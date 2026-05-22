#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>

namespace gconfig {

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    bool operator==(const Vec2& other) const = default;
};

enum class ValueType : std::uint8_t {
    Bool,
    Int,
    Float,
    String,
    Vec2,
};

using Value = std::variant<bool, int, float, std::string, Vec2>;

ValueType value_type(const Value& value);
const char* value_type_name(ValueType type);
const char* value_type_name(const Value& value);
bool same_value_type(const Value& a, const Value& b);

std::optional<bool> as_bool(const Value& value);
std::optional<int> as_int(const Value& value);
std::optional<float> as_float(const Value& value);
std::optional<std::string> as_string(const Value& value);
std::optional<Vec2> as_vec2(const Value& value);

bool value_equals(const Value& a, const Value& b);

} // namespace gconfig
