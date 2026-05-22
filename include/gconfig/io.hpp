#pragma once

#include "gconfig/document_list.hpp"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace gconfig {

struct Diagnostic {
    std::string message;
    int line = 1;
    int column = 1;
};

struct LoadDocumentResult {
    bool ok = false;
    Document document;
    ReconcileReport reconcile_report;
    std::vector<Diagnostic> diagnostics;
};

struct LoadDocumentListResult {
    bool ok = false;
    DocumentList documents;
    ReconcileReport reconcile_report;
    std::vector<Diagnostic> diagnostics;
};

LoadDocumentResult load_document_string(std::string_view text,
                                        std::string_view root_name = "config");
LoadDocumentResult load_document_string(std::string_view text, const Schema& schema,
                                        UnknownKeyMode unknown_key_mode = UnknownKeyMode::Preserve,
                                        std::string_view root_name = "config");
LoadDocumentListResult load_document_list_string(std::string_view text,
                                                 std::string_view root_name = "config_list",
                                                 std::string_view record_name = "document");
LoadDocumentListResult
load_document_list_string(std::string_view text, const Schema& schema,
                          UnknownKeyMode unknown_key_mode = UnknownKeyMode::Preserve,
                          std::string_view root_name = "config_list",
                          std::string_view record_name = "document");

bool save_document_string(const Document& document, std::string& out,
                          std::string_view root_name = "config");
bool save_document_string(const Document& document, const Schema& schema, std::string& out,
                          std::string_view root_name = "config");
bool save_document_list_string(const DocumentList& documents, std::string& out,
                               std::string_view root_name = "config_list",
                               std::string_view record_name = "document");
bool save_document_list_string(const DocumentList& documents, const Schema& schema,
                               std::string& out, std::string_view root_name = "config_list",
                               std::string_view record_name = "document");

LoadDocumentResult load_document_file(const std::filesystem::path& path,
                                      std::string_view root_name = "config");
LoadDocumentResult load_document_file(const std::filesystem::path& path, const Schema& schema,
                                      UnknownKeyMode unknown_key_mode = UnknownKeyMode::Preserve,
                                      std::string_view root_name = "config");
LoadDocumentListResult load_document_list_file(const std::filesystem::path& path,
                                               std::string_view root_name = "config_list",
                                               std::string_view record_name = "document");
LoadDocumentListResult
load_document_list_file(const std::filesystem::path& path, const Schema& schema,
                        UnknownKeyMode unknown_key_mode = UnknownKeyMode::Preserve,
                        std::string_view root_name = "config_list",
                        std::string_view record_name = "document");

bool save_document_file(const std::filesystem::path& path, const Document& document,
                        std::string_view root_name = "config");
bool save_document_file(const std::filesystem::path& path, const Document& document,
                        const Schema& schema, std::string_view root_name = "config");
bool save_document_list_file(const std::filesystem::path& path, const DocumentList& documents,
                             std::string_view root_name = "config_list",
                             std::string_view record_name = "document");
bool save_document_list_file(const std::filesystem::path& path, const DocumentList& documents,
                             const Schema& schema, std::string_view root_name = "config_list",
                             std::string_view record_name = "document");

} // namespace gconfig
