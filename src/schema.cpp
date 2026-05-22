#include "gconfig/schema.hpp"

#include <algorithm>
#include <utility>

namespace gconfig {

Field& Field::set_label(std::string text) {
    label = std::move(text);
    return *this;
}

Field& Field::set_description(std::string text) {
    description = std::move(text);
    return *this;
}

Field& Field::add_category(std::string category) {
    categories.push_back(std::move(category));
    return *this;
}

Field& Field::set_order(int value) {
    order = value;
    return *this;
}

Field& Field::set_range(float min, float max, float step) {
    range = Range{min, max, step};
    return *this;
}

Field& Field::add_option(std::string value, std::string option_label) {
    options.push_back(Option{std::move(value), std::move(option_label)});
    return *this;
}

Field& Schema::add_bool(std::string key, bool default_value) {
    Field field;
    field.key = std::move(key);
    field.default_value = default_value;
    return add_field(std::move(field));
}

Field& Schema::add_int(std::string key, int default_value) {
    Field field;
    field.key = std::move(key);
    field.default_value = default_value;
    return add_field(std::move(field));
}

Field& Schema::add_float(std::string key, float default_value) {
    Field field;
    field.key = std::move(key);
    field.default_value = default_value;
    return add_field(std::move(field));
}

Field& Schema::add_string(std::string key, std::string default_value) {
    Field field;
    field.key = std::move(key);
    field.default_value = std::move(default_value);
    return add_field(std::move(field));
}

Field& Schema::add_vec2(std::string key, Vec2 default_value) {
    Field field;
    field.key = std::move(key);
    field.default_value = default_value;
    return add_field(std::move(field));
}

Field& Schema::add_field(Field field) {
    auto it = std::find_if(fields_.begin(), fields_.end(),
                           [&](const Field& existing) { return existing.key == field.key; });
    if (it != fields_.end()) {
        *it = std::move(field);
        return *it;
    }
    fields_.push_back(std::move(field));
    return fields_.back();
}

const Field* Schema::find(std::string_view key) const {
    auto it = std::find_if(fields_.begin(), fields_.end(),
                           [&](const Field& field) { return field.key == key; });
    if (it == fields_.end())
        return nullptr;
    return &*it;
}

const std::vector<Field>& Schema::fields() const {
    return fields_;
}

} // namespace gconfig
