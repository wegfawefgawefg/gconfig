#include "gconfig/document.hpp"

#include <algorithm>
#include <utility>

namespace gconfig {

namespace {

std::string make_type_message(std::string_view key, const Value& actual, const Value& expected) {
    std::string message = "reset ";
    message += key;
    message += " from ";
    message += value_type_name(actual);
    message += " to default ";
    message += value_type_name(expected);
    return message;
}

bool clamp_float_value(Value& value, const Field& field) {
    if (!field.range)
        return false;
    if (float* number = std::get_if<float>(&value)) {
        float clamped = std::clamp(*number, field.range->min, field.range->max);
        if (clamped == *number)
            return false;
        *number = clamped;
        return true;
    }
    if (int* number = std::get_if<int>(&value)) {
        int min = static_cast<int>(field.range->min);
        int max = static_cast<int>(field.range->max);
        int clamped = std::clamp(*number, min, max);
        if (clamped == *number)
            return false;
        *number = clamped;
        return true;
    }
    return false;
}

bool has_valid_option(const Value& value, const Field& field) {
    if (field.options.empty())
        return true;
    const std::string* text = std::get_if<std::string>(&value);
    if (!text)
        return true;
    return std::any_of(field.options.begin(), field.options.end(),
                       [&](const Option& option) { return option.value == *text; });
}

bool report_contains_key(const ReconcileReport& report, std::string_view key) {
    return std::any_of(report.changes.begin(), report.changes.end(),
                       [&](const ReconcileChange& change) { return change.key == key; });
}

bool schema_contains_key(const Schema& schema, std::string_view key) {
    return schema.find(key) != nullptr;
}

void report_unknown_preserved(const Schema& schema,
                              const std::unordered_map<std::string, Value>& values,
                              ReconcileReport& report) {
    for (const auto& [key, value] : values) {
        (void)value;
        if (schema_contains_key(schema, key))
            continue;
        if (report_contains_key(report, key))
            continue;
        report.changes.push_back(ReconcileChange{ReconcileChangeKind::PreservedUnknown, key,
                                                 "preserved unknown key", false});
    }
}

bool clamp_field_value(Value& value, const Field& field) {
    return clamp_float_value(value, field);
}

bool reset_invalid_option(Value& value, const Field& field) {
    if (has_valid_option(value, field))
        return false;
    value = field.default_value;
    return true;
}

bool coerce_value_to_schema_type(Value& value, const Value& default_value) {
    if (std::holds_alternative<float>(default_value)) {
        if (const int* number = std::get_if<int>(&value)) {
            value = static_cast<float>(*number);
            return true;
        }
    }

    if (std::holds_alternative<bool>(default_value)) {
        if (const int* number = std::get_if<int>(&value)) {
            if (*number == 0 || *number == 1) {
                value = (*number != 0);
                return true;
            }
        }
    }

    return false;
}

} // namespace

bool ReconcileReport::changed() const {
    return std::any_of(changes.begin(), changes.end(),
                       [](const ReconcileChange& change) { return change.changed; });
}

bool Document::contains(std::string_view key) const {
    return find(key) != nullptr;
}

const Value* Document::find(std::string_view key) const {
    auto it = values_.find(std::string(key));
    if (it == values_.end())
        return nullptr;
    return &it->second;
}

Value* Document::find(std::string_view key) {
    auto it = values_.find(std::string(key));
    if (it == values_.end())
        return nullptr;
    return &it->second;
}

void Document::set(std::string key, Value value) {
    values_[std::move(key)] = std::move(value);
}

void Document::set_bool(std::string key, bool value) {
    set(std::move(key), value);
}

void Document::set_int(std::string key, int value) {
    set(std::move(key), value);
}

void Document::set_float(std::string key, float value) {
    set(std::move(key), value);
}

void Document::set_string(std::string key, std::string value) {
    set(std::move(key), std::move(value));
}

void Document::set_vec2(std::string key, Vec2 value) {
    set(std::move(key), value);
}

bool Document::get_bool(std::string_view key, bool fallback) const {
    const Value* value = find(key);
    if (!value)
        return fallback;
    return as_bool(*value).value_or(fallback);
}

int Document::get_int(std::string_view key, int fallback) const {
    const Value* value = find(key);
    if (!value)
        return fallback;
    return as_int(*value).value_or(fallback);
}

float Document::get_float(std::string_view key, float fallback) const {
    const Value* value = find(key);
    if (!value)
        return fallback;
    return as_float(*value).value_or(fallback);
}

std::string Document::get_string(std::string_view key, std::string fallback) const {
    const Value* value = find(key);
    if (!value)
        return fallback;
    return as_string(*value).value_or(std::move(fallback));
}

Vec2 Document::get_vec2(std::string_view key, Vec2 fallback) const {
    const Value* value = find(key);
    if (!value)
        return fallback;
    return as_vec2(*value).value_or(fallback);
}

ReconcileReport Document::reconcile(const Schema& schema, UnknownKeyMode unknown_key_mode) {
    ReconcileReport report;

    for (const Field& field : schema.fields()) {
        auto it = values_.find(field.key);
        if (it == values_.end()) {
            values_[field.key] = field.default_value;
            report.changes.push_back(
                ReconcileChange{ReconcileChangeKind::AddedDefault, field.key, "added default"});
            continue;
        }

        if (!same_value_type(it->second, field.default_value) &&
            coerce_value_to_schema_type(it->second, field.default_value)) {
            report.changes.push_back(
                ReconcileChange{ReconcileChangeKind::Coerced, field.key, "coerced to schema type"});
        }

        if (!same_value_type(it->second, field.default_value)) {
            std::string message = make_type_message(field.key, it->second, field.default_value);
            it->second = field.default_value;
            report.changes.push_back(ReconcileChange{ReconcileChangeKind::ResetWrongType, field.key,
                                                     std::move(message)});
            continue;
        }

        if (reset_invalid_option(it->second, field)) {
            report.changes.push_back(ReconcileChange{ReconcileChangeKind::ResetInvalidOption,
                                                     field.key, "reset invalid option"});
        }

        if (clamp_field_value(it->second, field)) {
            report.changes.push_back(
                ReconcileChange{ReconcileChangeKind::Clamped, field.key, "clamped to range"});
        }
    }

    if (unknown_key_mode == UnknownKeyMode::Drop) {
        for (auto it = values_.begin(); it != values_.end();) {
            if (schema.find(it->first)) {
                ++it;
                continue;
            }
            std::string key = it->first;
            it = values_.erase(it);
            report.changes.push_back(
                ReconcileChange{ReconcileChangeKind::DroppedUnknown, key, "dropped unknown key"});
        }
    } else {
        report_unknown_preserved(schema, values_, report);
    }

    return report;
}

const std::unordered_map<std::string, Value>& Document::values() const {
    return values_;
}

std::unordered_map<std::string, Value>& Document::values() {
    return values_;
}

} // namespace gconfig
