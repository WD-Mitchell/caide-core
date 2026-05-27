#include "naming.h"

#include <atomic>

namespace caide::references {
namespace {

std::atomic<uint64_t> g_reference_counter {1};

std::string generate_id(const std::string& label) {
    std::ostringstream stream;
    stream << (label.empty() ? "ref" : lowercase_copy(label)) << '-' << g_reference_counter.fetch_add(1, std::memory_order_relaxed);
    return stream.str();
}

}  // namespace

Result<std::shared_ptr<ReferenceData>> create_reference(const std::shared_ptr<ShapeData>& shape, const std::string& label) {
    auto required = require_shape(shape, "shape");
    if (!required) {
        return {required.status, nullptr};
    }
    auto reference = std::make_shared<ReferenceData>();
    reference->label = label;
    reference->shape = shape;
    reference->id = !shape->reference_id.empty() ? shape->reference_id : generate_id(label);
    reference->valid = true;
    if (shape->reference_id.empty()) {
        shape->reference_id = reference->id;
    }
    clear_error();
    return {Status::ok(), reference};
}

Result<std::shared_ptr<ShapeData>> resolve(DocumentData& doc, const std::string& ref_id) {
    return resolve_reference(doc, ref_id);
}

int is_reference_valid(const std::shared_ptr<ReferenceData>& reference) {
    if (!reference || !reference->valid) {
        return 0;
    }
    const auto shape = reference->shape.lock();
    return shape && shape->valid ? 1 : 0;
}

}  // namespace caide::references
