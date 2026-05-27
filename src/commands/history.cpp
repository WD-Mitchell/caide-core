#include "history.h"

namespace caide::commands {

Status undo(DocumentData& doc) {
    std::lock_guard<std::mutex> lock(doc.mutex);
    if (doc.history.empty()) {
        return set_error(CAIDE_ERR_HISTORY_EMPTY, "history is empty");
    }
    HistoryEntry entry = doc.history.back();
    doc.history.pop_back();
    doc.redo_stack.push_back(entry);
    if (entry.result && !entry.result->reference_id.empty()) {
        doc.shapes_by_ref.erase(entry.result->reference_id);
    }
    clear_error();
    return Status::ok();
}

Status redo(DocumentData& doc) {
    std::lock_guard<std::mutex> lock(doc.mutex);
    if (doc.redo_stack.empty()) {
        return set_error(CAIDE_ERR_HISTORY_EMPTY, "redo stack is empty");
    }
    HistoryEntry entry = doc.redo_stack.back();
    doc.redo_stack.pop_back();
    if (entry.result && !entry.result->reference_id.empty()) {
        doc.shapes_by_ref[entry.result->reference_id] = entry.result;
    }
    doc.history.push_back(entry);
    clear_error();
    return Status::ok();
}

int history_count(DocumentData& doc) {
    std::lock_guard<std::mutex> lock(doc.mutex);
    return static_cast<int>(doc.history.size());
}

Result<std::shared_ptr<ShapeData>> find_shape(DocumentData& doc, const std::string& ref_id) {
    std::lock_guard<std::mutex> lock(doc.mutex);
    return resolve_reference(doc, ref_id);
}

}  // namespace caide::commands
