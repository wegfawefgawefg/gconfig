#include "gconfig/io.hpp"

#include "gsexp/sexp.hpp"

#include <algorithm>
#include <charconv>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <utility>

namespace gconfig {

namespace {

void copy_parse_diagnostics(const gsexp::ParseResult& parsed, std::vector<Diagnostic>& out) {
    for (const gsexp::Diagnostic& diag : parsed.diagnostics) {
        out.push_back(Diagnostic{diag.message, diag.line, diag.column});
    }
}

std::optional<int> parse_int_atom(std::string_view text) {
    int value = 0;
    const char* begin = text.data();
    const char* end = text.data() + text.size();
    auto result = std::from_chars(begin, end, value);
    if (result.ec != std::errc{} || result.ptr != end)
        return std::nullopt;
    return value;
}

std::optional<float> parse_float_atom(std::string_view text) {
    std::string copy(text);
    char* end = nullptr;
    float value = std::strtof(copy.c_str(), &end);
    if (end != copy.c_str() + copy.size())
        return std::nullopt;
    return value;
}

std::optional<Value> parse_value(gsexp::Node node) {
    if (!node.valid())
        return std::nullopt;
    if (node.is_string())
        return std::string(node.text());
    if (node.type() == gsexp::ValueType::Atom) {
        std::string_view text = node.text();
        if (text == "true")
            return true;
        if (text == "false")
            return false;
        if (gsexp::looks_like_integer(text)) {
            if (std::optional<int> value = parse_int_atom(text))
                return *value;
        }
        if (gsexp::looks_like_float(text)) {
            if (std::optional<float> value = parse_float_atom(text))
                return *value;
        }
        return std::string(text);
    }
    if (node.is_list() && node.child_count() == 3 && node.head().is_atom("vec2")) {
        std::optional<float> x = parse_float_atom(node.child_at(1).text());
        std::optional<float> y = parse_float_atom(node.child_at(2).text());
        if (x && y)
            return Vec2{*x, *y};
    }
    return std::nullopt;
}

void parse_values_node(gsexp::Node values_node, Document& document) {
    for (gsexp::Node child : values_node.children()) {
        if (!child.is_list() || child.child_count() < 3 || !child.head().is_atom("key"))
            continue;
        gsexp::Node key_node = child.child_at(1);
        if (!key_node.is_string())
            continue;
        std::optional<Value> value = parse_value(child.child_at(2));
        if (!value)
            continue;
        document.set(std::string(key_node.text()), std::move(*value));
    }
}

void parse_direct_record_values(gsexp::Node record_node, Document& document) {
    for (gsexp::Node child : record_node.children()) {
        if (!child.is_list() || child.child_count() < 2)
            continue;
        gsexp::Node key_node = child.head();
        if (!key_node.valid() || key_node.type() != gsexp::ValueType::Atom)
            continue;
        std::string_view key = key_node.text();
        if (key == "id" || key == "name" || key == "values")
            continue;
        std::optional<Value> value = parse_value(child.child_at(1));
        if (!value)
            continue;
        document.set(std::string(key), std::move(*value));
    }
}

gsexp::Node find_root(const gsexp::ParseResult& parsed, std::string_view root_name) {
    for (std::size_t index = 0; index < parsed.root_count(); ++index) {
        gsexp::Node root = parsed.root(index);
        if (root.is_list() && root.head().is_atom(root_name))
            return root;
    }
    return {};
}

void append_value(std::ostream& out, const Value& value) {
    std::visit(
        [&out](const auto& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, bool>) {
                out << (arg ? "true" : "false");
            } else if constexpr (std::is_same_v<T, int>) {
                out << arg;
            } else if constexpr (std::is_same_v<T, float>) {
                out << arg;
            } else if constexpr (std::is_same_v<T, std::string>) {
                out << gsexp::quote_string(arg);
            } else if constexpr (std::is_same_v<T, Vec2>) {
                out << "(vec2 " << arg.x << " " << arg.y << ")";
            }
        },
        value);
}

void append_values(std::ostream& out, const Document& document, int indent) {
    std::string pad(static_cast<std::size_t>(indent), ' ');
    out << pad << "(values\n";
    for (const auto& [key, value] : document.values()) {
        out << pad << "  (key " << gsexp::quote_string(key) << " ";
        append_value(out, value);
        out << ")\n";
    }
    out << pad << ")\n";
}

std::vector<std::string> ordered_keys(const Document& document, const Schema& schema) {
    std::vector<const Field*> fields;
    fields.reserve(schema.fields().size());
    for (const Field& field : schema.fields()) {
        if (document.contains(field.key))
            fields.push_back(&field);
    }
    std::stable_sort(fields.begin(), fields.end(), [](const Field* a, const Field* b) {
        if (a->order != b->order)
            return a->order < b->order;
        return a->key < b->key;
    });

    std::vector<std::string> keys;
    keys.reserve(document.values().size());
    for (const Field* field : fields)
        keys.push_back(field->key);

    std::set<std::string> unknown_keys;
    for (const auto& [key, value] : document.values()) {
        (void)value;
        if (!schema.find(key))
            unknown_keys.insert(key);
    }
    for (const std::string& key : unknown_keys)
        keys.push_back(key);
    return keys;
}

void append_values_ordered(std::ostream& out, const Document& document, const Schema& schema,
                           int indent) {
    std::string pad(static_cast<std::size_t>(indent), ' ');
    out << pad << "(values\n";
    for (const std::string& key : ordered_keys(document, schema)) {
        const Value* value = document.find(key);
        if (!value)
            continue;
        out << pad << "  (key " << gsexp::quote_string(key) << " ";
        append_value(out, *value);
        out << ")\n";
    }
    out << pad << ")\n";
}

std::optional<std::string> read_file(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input.is_open())
        return std::nullopt;
    std::ostringstream out;
    out << input.rdbuf();
    return out.str();
}

bool write_file(const std::filesystem::path& path, std::string_view text) {
    if (path.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
        if (ec)
            return false;
    }
    std::ofstream output(path, std::ios::trunc);
    if (!output.is_open())
        return false;
    output << text;
    return output.good();
}

} // namespace

LoadDocumentResult load_document_string(std::string_view text, std::string_view root_name) {
    LoadDocumentResult result;
    gsexp::ParseResult parsed = gsexp::parse(text);
    if (!parsed.ok) {
        copy_parse_diagnostics(parsed, result.diagnostics);
        return result;
    }

    gsexp::Node root = find_root(parsed, root_name);
    if (!root.valid()) {
        result.diagnostics.push_back(Diagnostic{"missing root form", 1, 1});
        return result;
    }

    gsexp::Node values = gsexp::FormView(root).find("values");
    if (values.valid())
        parse_values_node(values, result.document);
    result.ok = true;
    return result;
}

LoadDocumentResult load_document_string(std::string_view text, const Schema& schema,
                                        UnknownKeyMode unknown_key_mode,
                                        std::string_view root_name) {
    LoadDocumentResult result = load_document_string(text, root_name);
    if (result.ok)
        result.reconcile_report = result.document.reconcile(schema, unknown_key_mode);
    return result;
}

LoadDocumentListResult load_document_list_string(std::string_view text, std::string_view root_name,
                                                 std::string_view record_name) {
    LoadDocumentListResult result;
    gsexp::ParseResult parsed = gsexp::parse(text);
    if (!parsed.ok) {
        copy_parse_diagnostics(parsed, result.diagnostics);
        return result;
    }

    gsexp::Node root = find_root(parsed, root_name);
    if (!root.valid()) {
        result.diagnostics.push_back(Diagnostic{"missing root form", 1, 1});
        return result;
    }

    for (gsexp::Node child : root.children()) {
        if (!child.is_list() || !child.head().is_atom(record_name))
            continue;
        gsexp::FormView view(child);
        std::optional<int> id = view.get_int("id");
        std::optional<std::string> name = view.get_string("name");
        if (!id || !name)
            continue;
        DocumentRecord record;
        record.id = *id;
        record.name = *name;
        gsexp::Node values = view.find("values");
        if (values.valid())
            parse_values_node(values, record.document);
        parse_direct_record_values(child, record.document);
        result.documents.add(std::move(record));
    }

    result.ok = true;
    return result;
}

LoadDocumentListResult load_document_list_string(std::string_view text, const Schema& schema,
                                                 UnknownKeyMode unknown_key_mode,
                                                 std::string_view root_name,
                                                 std::string_view record_name) {
    LoadDocumentListResult result = load_document_list_string(text, root_name, record_name);
    if (result.ok)
        result.reconcile_report = result.documents.reconcile_all(schema, unknown_key_mode);
    return result;
}

bool save_document_string(const Document& document, std::string& out, std::string_view root_name) {
    std::ostringstream stream;
    stream << "(" << root_name << "\n";
    append_values(stream, document, 2);
    stream << ")\n";
    out = stream.str();
    return true;
}

bool save_document_string(const Document& document, const Schema& schema, std::string& out,
                          std::string_view root_name) {
    std::ostringstream stream;
    stream << "(" << root_name << "\n";
    append_values_ordered(stream, document, schema, 2);
    stream << ")\n";
    out = stream.str();
    return true;
}

bool save_document_list_string(const DocumentList& documents, std::string& out,
                               std::string_view root_name, std::string_view record_name) {
    std::ostringstream stream;
    stream << "(" << root_name << "\n";
    for (const DocumentRecord& record : documents.records()) {
        stream << "  (" << record_name << "\n";
        stream << "    (id " << record.id << ")\n";
        stream << "    (name " << gsexp::quote_string(record.name) << ")\n";
        append_values(stream, record.document, 4);
        stream << "  )\n";
    }
    stream << ")\n";
    out = stream.str();
    return true;
}

bool save_document_list_string(const DocumentList& documents, const Schema& schema,
                               std::string& out, std::string_view root_name,
                               std::string_view record_name) {
    std::ostringstream stream;
    stream << "(" << root_name << "\n";
    for (const DocumentRecord& record : documents.records()) {
        stream << "  (" << record_name << "\n";
        stream << "    (id " << record.id << ")\n";
        stream << "    (name " << gsexp::quote_string(record.name) << ")\n";
        append_values_ordered(stream, record.document, schema, 4);
        stream << "  )\n";
    }
    stream << ")\n";
    out = stream.str();
    return true;
}

LoadDocumentResult load_document_file(const std::filesystem::path& path,
                                      std::string_view root_name) {
    std::optional<std::string> text = read_file(path);
    if (!text)
        return LoadDocumentResult{true, {}, {}, {}};
    return load_document_string(*text, root_name);
}

LoadDocumentResult load_document_file(const std::filesystem::path& path, const Schema& schema,
                                      UnknownKeyMode unknown_key_mode, std::string_view root_name) {
    LoadDocumentResult result = load_document_file(path, root_name);
    if (result.ok)
        result.reconcile_report = result.document.reconcile(schema, unknown_key_mode);
    return result;
}

LoadDocumentListResult load_document_list_file(const std::filesystem::path& path,
                                               std::string_view root_name,
                                               std::string_view record_name) {
    std::optional<std::string> text = read_file(path);
    if (!text)
        return LoadDocumentListResult{true, {}, {}, {}};
    return load_document_list_string(*text, root_name, record_name);
}

LoadDocumentListResult load_document_list_file(const std::filesystem::path& path,
                                               const Schema& schema,
                                               UnknownKeyMode unknown_key_mode,
                                               std::string_view root_name,
                                               std::string_view record_name) {
    LoadDocumentListResult result = load_document_list_file(path, root_name, record_name);
    if (result.ok)
        result.reconcile_report = result.documents.reconcile_all(schema, unknown_key_mode);
    return result;
}

bool save_document_file(const std::filesystem::path& path, const Document& document,
                        std::string_view root_name) {
    std::string text;
    if (!save_document_string(document, text, root_name))
        return false;
    return write_file(path, text);
}

bool save_document_file(const std::filesystem::path& path, const Document& document,
                        const Schema& schema, std::string_view root_name) {
    std::string text;
    if (!save_document_string(document, schema, text, root_name))
        return false;
    return write_file(path, text);
}

bool save_document_list_file(const std::filesystem::path& path, const DocumentList& documents,
                             std::string_view root_name, std::string_view record_name) {
    std::string text;
    if (!save_document_list_string(documents, text, root_name, record_name))
        return false;
    return write_file(path, text);
}

bool save_document_list_file(const std::filesystem::path& path, const DocumentList& documents,
                             const Schema& schema, std::string_view root_name,
                             std::string_view record_name) {
    std::string text;
    if (!save_document_list_string(documents, schema, text, root_name, record_name))
        return false;
    return write_file(path, text);
}

} // namespace gconfig
