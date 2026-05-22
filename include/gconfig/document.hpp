#pragma once

#include "gconfig/schema.hpp"

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace gconfig {

enum class UnknownKeyMode {
    Preserve,
    Drop,
};

enum class ReconcileChangeKind {
    AddedDefault,
    Coerced,
    ResetWrongType,
    Clamped,
    ResetInvalidOption,
    PreservedUnknown,
    DroppedUnknown,
};

struct ReconcileChange {
    ReconcileChangeKind kind = ReconcileChangeKind::AddedDefault;
    std::string key;
    std::string message;
    bool changed = true;
};

struct ReconcileReport {
    std::vector<ReconcileChange> changes;

    bool changed() const;
};

class Document {
  public:
    bool contains(std::string_view key) const;
    const Value* find(std::string_view key) const;
    Value* find(std::string_view key);

    void set(std::string key, Value value);
    void set_bool(std::string key, bool value);
    void set_int(std::string key, int value);
    void set_float(std::string key, float value);
    void set_string(std::string key, std::string value);
    void set_vec2(std::string key, Vec2 value);

    bool get_bool(std::string_view key, bool fallback = false) const;
    int get_int(std::string_view key, int fallback = 0) const;
    float get_float(std::string_view key, float fallback = 0.0f) const;
    std::string get_string(std::string_view key, std::string fallback = {}) const;
    Vec2 get_vec2(std::string_view key, Vec2 fallback = {}) const;

    ReconcileReport reconcile(const Schema& schema,
                              UnknownKeyMode unknown_key_mode = UnknownKeyMode::Preserve);

    const std::unordered_map<std::string, Value>& values() const;
    std::unordered_map<std::string, Value>& values();

  private:
    std::unordered_map<std::string, Value> values_;
};

} // namespace gconfig
