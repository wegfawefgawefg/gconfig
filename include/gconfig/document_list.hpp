#pragma once

#include "gconfig/document.hpp"

#include <string>
#include <vector>

namespace gconfig {

struct DocumentRecord {
    int id = -1;
    std::string name;
    Document document;
};

class DocumentList {
  public:
    DocumentRecord& add(DocumentRecord record);
    DocumentRecord* find(int id);
    const DocumentRecord* find(int id) const;
    bool remove(int id);
    bool contains(int id) const;

    ReconcileReport reconcile_all(const Schema& schema,
                                  UnknownKeyMode unknown_key_mode = UnknownKeyMode::Preserve);

    const std::vector<DocumentRecord>& records() const;
    std::vector<DocumentRecord>& records();

  private:
    std::vector<DocumentRecord> records_;
};

} // namespace gconfig
