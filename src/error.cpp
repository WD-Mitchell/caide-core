#include "error.h"

#include <cctype>

namespace caide {
namespace {

struct ErrorState {
    CaideError code {CAIDE_OK};
    std::string message {};
};

thread_local ErrorState g_error_state {};

}  // namespace

Status set_error(CaideError code, const std::string& message) {
    g_error_state.code = code;
    g_error_state.message = message;
    return Status::error(code, message);
}

void clear_error() {
    g_error_state.code = CAIDE_OK;
    g_error_state.message.clear();
}

const char* last_error_message() {
    return g_error_state.message.empty() ? "" : g_error_state.message.c_str();
}

CaideError last_error_code() {
    return g_error_state.code;
}

Result<std::shared_ptr<ShapeData>> require_shape(const std::shared_ptr<ShapeData>& shape, const char* name) {
    if (!shape) {
        return {set_error(CAIDE_ERR_NULL_HANDLE, std::string(name) + " is null"), nullptr};
    }
    if (!shape->valid) {
        return {set_error(CAIDE_ERR_OPERATION_FAILED, std::string(name) + " is invalid"), nullptr};
    }
    return {Status::ok(), shape};
}

Result<std::shared_ptr<DocumentData>> require_document(const std::shared_ptr<DocumentData>& doc, const char* name) {
    if (!doc) {
        return {set_error(CAIDE_ERR_NULL_HANDLE, std::string(name) + " is null"), nullptr};
    }
    return {Status::ok(), doc};
}

Result<BoundingBox> require_valid_bbox(const BoundingBox& bbox, const char* label) {
    if (!bbox_is_valid(bbox)) {
        return {set_error(CAIDE_ERR_INVALID_PARAM, std::string(label) + " produced an invalid bounding box"), {}};
    }
    return {Status::ok(), bbox};
}

std::shared_ptr<ShapeData> make_shape(ShapeKind kind, const std::string& name, const BoundingBox& bbox, double volume, double area) {
    auto shape = std::make_shared<ShapeData>();
    shape->kind = kind;
    shape->debug_name = name;
    shape->bbox = bbox;
    shape->volume = std::max(0.0, volume);
    shape->surface_area = std::max(0.0, area);
    shape->valid = bbox_is_valid(bbox);
    shape->closed = kind != ShapeKind::Wire;
    shape->transform = Matrix4::identity();
    shape->sampled_points = bbox_corners(bbox);
    shape->history_tags.push_back(shape_kind_name(kind));
    return shape;
}

std::shared_ptr<MeshData> make_mesh() {
    return std::make_shared<MeshData>();
}

Result<std::shared_ptr<ShapeData>> clone_with_transform(const std::shared_ptr<ShapeData>& shape, const Matrix4& matrix, const std::string& reason) {
    auto required = require_shape(shape, "shape");
    if (!required) {
        return {required.status, nullptr};
    }

    auto clone = clone_shape_data(shape);
    clone->transform = matrix * shape->transform;
    clone->bbox = transform_bbox(shape->bbox, matrix);
    clone->history_tags.push_back(reason);
    clone->metadata["transform_reason"] = reason;
    clone->sampled_points.clear();
    for (const Vec3& point : shape->sampled_points) {
        clone->sampled_points.push_back(transform_point(matrix, point));
    }
    if (clone->sampled_points.empty()) {
        clone->sampled_points = bbox_corners(clone->bbox);
    }
#ifndef CAIDE_STUB_MODE
    clone->native_shape = shape->native_shape;
#endif
    clear_error();
    return {Status::ok(), clone};
}

void push_history(DocumentData& doc, const std::string& type, const std::string& params_json, const std::string& target_ref, const std::shared_ptr<ShapeData>& shape) {
    HistoryEntry entry;
    entry.id = next_reference_id(doc, "hist");
    entry.type = type;
    entry.params_json = params_json;
    entry.target_ref = target_ref;
    entry.result = shape;
    doc.history.push_back(entry);
    doc.redo_stack.clear();
}

Result<std::shared_ptr<ShapeData>> resolve_reference(DocumentData& doc, const std::string& ref_id) {
    const auto it = doc.shapes_by_ref.find(ref_id);
    if (it == doc.shapes_by_ref.end()) {
        return {set_error(CAIDE_ERR_REFERENCE_NOT_FOUND, "Unknown reference: " + ref_id), nullptr};
    }
    if (!it->second || !it->second->valid) {
        return {set_error(CAIDE_ERR_REFERENCE_NOT_FOUND, "Reference is no longer valid: " + ref_id), nullptr};
    }
    clear_error();
    return {Status::ok(), it->second};
}

std::string trim_copy(const std::string& value) {
    std::size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])) != 0) {
        ++start;
    }
    std::size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
        --end;
    }
    return value.substr(start, end - start);
}

std::string lowercase_copy(const std::string& value) {
    std::string out = value;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return out;
}

std::vector<std::string> split_csv(const std::string& value) {
    std::vector<std::string> parts;
    std::string current;
    int bracket_depth = 0;
    bool in_quotes = false;
    for (char ch : value) {
        if (ch == '"') {
            in_quotes = !in_quotes;
            current.push_back(ch);
            continue;
        }
        if (!in_quotes) {
            if (ch == '[' || ch == '{') {
                ++bracket_depth;
            } else if (ch == ']' || ch == '}') {
                --bracket_depth;
            } else if (ch == ',' && bracket_depth == 0) {
                parts.push_back(trim_copy(current));
                current.clear();
                continue;
            }
        }
        current.push_back(ch);
    }
    if (!current.empty()) {
        parts.push_back(trim_copy(current));
    }
    return parts;
}

}  // namespace caide
