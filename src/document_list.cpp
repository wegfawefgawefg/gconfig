#include "gconfig/document_list.hpp"

#include <algorithm>
#include <utility>

namespace gconfig {

DocumentRecord& DocumentList::add(DocumentRecord record) {
    if (DocumentRecord* existing = find(record.id)) {
        *existing = std::move(record);
        return *existing;
    }
    records_.push_back(std::move(record));
    return records_.back();
}

DocumentRecord* DocumentList::find(int id) {
    auto it = std::find_if(records_.begin(), records_.end(),
                           [&](const DocumentRecord& record) { return record.id == id; });
    if (it == records_.end())
        return nullptr;
    return &*it;
}

const DocumentRecord* DocumentList::find(int id) const {
    auto it = std::find_if(records_.begin(), records_.end(),
                           [&](const DocumentRecord& record) { return record.id == id; });
    if (it == records_.end())
        return nullptr;
    return &*it;
}

bool DocumentList::remove(int id) {
    auto it = std::remove_if(records_.begin(), records_.end(),
                             [&](const DocumentRecord& record) { return record.id == id; });
    if (it == records_.end())
        return false;
    records_.erase(it, records_.end());
    return true;
}

bool DocumentList::contains(int id) const {
    return find(id) != nullptr;
}

ReconcileReport DocumentList::reconcile_all(const Schema& schema, UnknownKeyMode unknown_key_mode) {
    ReconcileReport combined;
    for (DocumentRecord& record : records_) {
        ReconcileReport report = record.document.reconcile(schema, unknown_key_mode);
        for (ReconcileChange change : report.changes) {
            change.message = "document " + std::to_string(record.id) + ": " + change.message;
            combined.changes.push_back(std::move(change));
        }
    }
    return combined;
}

const std::vector<DocumentRecord>& DocumentList::records() const {
    return records_;
}

std::vector<DocumentRecord>& DocumentList::records() {
    return records_;
}

} // namespace gconfig
