#pragma once

#include "gconfig/value.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace gconfig {

struct Option {
    std::string value;
    std::string label;
};

struct Range {
    float min = 0.0f;
    float max = 0.0f;
    float step = 0.0f;
};

struct Field {
    std::string key;
    Value default_value = false;
    std::string label;
    std::string description;
    std::vector<std::string> categories;
    int order = 0;
    std::optional<Range> range;
    std::vector<Option> options;

    Field& set_label(std::string text);
    Field& set_description(std::string text);
    Field& add_category(std::string category);
    Field& set_order(int value);
    Field& set_range(float min, float max, float step = 0.0f);
    Field& add_option(std::string value, std::string option_label);
};

class Schema {
  public:
    Field& add_bool(std::string key, bool default_value);
    Field& add_int(std::string key, int default_value);
    Field& add_float(std::string key, float default_value);
    Field& add_string(std::string key, std::string default_value);
    Field& add_vec2(std::string key, Vec2 default_value);
    Field& add_field(Field field);

    const Field* find(std::string_view key) const;
    const std::vector<Field>& fields() const;

  private:
    std::vector<Field> fields_;
};

} // namespace gconfig
