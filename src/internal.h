#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "caide_core/caide_core.h"

#ifndef CAIDE_STUB_MODE
#include <TopoDS_Shape.hxx>
#endif

namespace caide {

constexpr double kPi = 3.1415926535897932384626433832795;
constexpr double kDefaultTolerance = 1e-6;

struct Vec3 {
    double x {0.0};
    double y {0.0};
    double z {0.0};
};

struct BoundingBox {
    Vec3 min {0.0, 0.0, 0.0};
    Vec3 max {0.0, 0.0, 0.0};
};

inline Vec3 operator+(const Vec3& a, const Vec3& b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 operator-(const Vec3& a, const Vec3& b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 operator*(const Vec3& a, double scalar) { return {a.x * scalar, a.y * scalar, a.z * scalar}; }
inline Vec3 operator/(const Vec3& a, double scalar) { return {a.x / scalar, a.y / scalar, a.z / scalar}; }

inline double dot(const Vec3& a, const Vec3& b) { return (a.x * b.x) + (a.y * b.y) + (a.z * b.z); }
inline Vec3 cross(const Vec3& a, const Vec3& b) {
    return {
        (a.y * b.z) - (a.z * b.y),
        (a.z * b.x) - (a.x * b.z),
        (a.x * b.y) - (a.y * b.x),
    };
}
inline double length(const Vec3& v) { return std::sqrt(dot(v, v)); }
inline Vec3 normalized(const Vec3& v) {
    const double len = length(v);
    if (len <= kDefaultTolerance) {
        return {0.0, 0.0, 1.0};
    }
    return v / len;
}

inline BoundingBox make_bbox(const Vec3& min, const Vec3& max) {
    return {
        {std::min(min.x, max.x), std::min(min.y, max.y), std::min(min.z, max.z)},
        {std::max(min.x, max.x), std::max(min.y, max.y), std::max(min.z, max.z)},
    };
}

inline BoundingBox union_bbox(const BoundingBox& a, const BoundingBox& b) {
    return {
        {std::min(a.min.x, b.min.x), std::min(a.min.y, b.min.y), std::min(a.min.z, b.min.z)},
        {std::max(a.max.x, b.max.x), std::max(a.max.y, b.max.y), std::max(a.max.z, b.max.z)},
    };
}

inline BoundingBox intersect_bbox(const BoundingBox& a, const BoundingBox& b) {
    return {
        {std::max(a.min.x, b.min.x), std::max(a.min.y, b.min.y), std::max(a.min.z, b.min.z)},
        {std::min(a.max.x, b.max.x), std::min(a.max.y, b.max.y), std::min(a.max.z, b.max.z)},
    };
}

inline bool bbox_is_valid(const BoundingBox& bbox) {
    return bbox.min.x <= bbox.max.x && bbox.min.y <= bbox.max.y && bbox.min.z <= bbox.max.z;
}

inline Vec3 bbox_size(const BoundingBox& bbox) {
    return {bbox.max.x - bbox.min.x, bbox.max.y - bbox.min.y, bbox.max.z - bbox.min.z};
}

inline Vec3 bbox_center(const BoundingBox& bbox) {
    return {(bbox.min.x + bbox.max.x) * 0.5, (bbox.min.y + bbox.max.y) * 0.5, (bbox.min.z + bbox.max.z) * 0.5};
}

inline double bbox_volume(const BoundingBox& bbox) {
    const auto size = bbox_size(bbox);
    return std::max(0.0, size.x) * std::max(0.0, size.y) * std::max(0.0, size.z);
}

inline double bbox_surface_area(const BoundingBox& bbox) {
    const auto size = bbox_size(bbox);
    return 2.0 * ((size.x * size.y) + (size.x * size.z) + (size.y * size.z));
}

struct Matrix4 {
    std::array<double, 16> m {};

    static Matrix4 identity() {
        Matrix4 value;
        value.m = {1.0, 0.0, 0.0, 0.0,
                   0.0, 1.0, 0.0, 0.0,
                   0.0, 0.0, 1.0, 0.0,
                   0.0, 0.0, 0.0, 1.0};
        return value;
    }

    static Matrix4 translation(const Vec3& v) {
        Matrix4 result = identity();
        result.m[12] = v.x;
        result.m[13] = v.y;
        result.m[14] = v.z;
        return result;
    }

    static Matrix4 scale(const Vec3& center, double factor) {
        Matrix4 result = identity();
        result.m[0] = factor;
        result.m[5] = factor;
        result.m[10] = factor;
        result.m[12] = center.x * (1.0 - factor);
        result.m[13] = center.y * (1.0 - factor);
        result.m[14] = center.z * (1.0 - factor);
        return result;
    }

    static Matrix4 rotation(Vec3 axis, double degrees) {
        axis = normalized(axis);
        const double radians = degrees * kPi / 180.0;
        const double c = std::cos(radians);
        const double s = std::sin(radians);
        const double t = 1.0 - c;
        const double x = axis.x;
        const double y = axis.y;
        const double z = axis.z;

        Matrix4 result = identity();
        result.m = {
            t * x * x + c,       t * x * y + s * z,   t * x * z - s * y,   0.0,
            t * x * y - s * z,   t * y * y + c,       t * y * z + s * x,   0.0,
            t * x * z + s * y,   t * y * z - s * x,   t * z * z + c,       0.0,
            0.0,                 0.0,                 0.0,                 1.0,
        };
        return result;
    }

    static Matrix4 mirror(Vec3 normal, double d) {
        normal = normalized(normal);
        const double nx = normal.x;
        const double ny = normal.y;
        const double nz = normal.z;
        Matrix4 result = identity();
        result.m = {
            1.0 - 2.0 * nx * nx, -2.0 * nx * ny,      -2.0 * nx * nz,      0.0,
            -2.0 * ny * nx,      1.0 - 2.0 * ny * ny, -2.0 * ny * nz,      0.0,
            -2.0 * nz * nx,      -2.0 * nz * ny,      1.0 - 2.0 * nz * nz, 0.0,
            -2.0 * d * nx,       -2.0 * d * ny,       -2.0 * d * nz,       1.0,
        };
        return result;
    }
};

inline Matrix4 operator*(const Matrix4& a, const Matrix4& b) {
    Matrix4 result {};
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            double value = 0.0;
            for (int k = 0; k < 4; ++k) {
                value += a.m[(k * 4) + row] * b.m[(col * 4) + k];
            }
            result.m[(col * 4) + row] = value;
        }
    }
    return result;
}

inline Vec3 transform_point(const Matrix4& matrix, const Vec3& point) {
    return {
        (matrix.m[0] * point.x) + (matrix.m[4] * point.y) + (matrix.m[8] * point.z) + matrix.m[12],
        (matrix.m[1] * point.x) + (matrix.m[5] * point.y) + (matrix.m[9] * point.z) + matrix.m[13],
        (matrix.m[2] * point.x) + (matrix.m[6] * point.y) + (matrix.m[10] * point.z) + matrix.m[14],
    };
}

inline BoundingBox transform_bbox(const BoundingBox& bbox, const Matrix4& matrix) {
    std::array<Vec3, 8> corners = {
        Vec3{bbox.min.x, bbox.min.y, bbox.min.z}, Vec3{bbox.max.x, bbox.min.y, bbox.min.z},
        Vec3{bbox.min.x, bbox.max.y, bbox.min.z}, Vec3{bbox.max.x, bbox.max.y, bbox.min.z},
        Vec3{bbox.min.x, bbox.min.y, bbox.max.z}, Vec3{bbox.max.x, bbox.min.y, bbox.max.z},
        Vec3{bbox.min.x, bbox.max.y, bbox.max.z}, Vec3{bbox.max.x, bbox.max.y, bbox.max.z},
    };

    BoundingBox out {{ std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), std::numeric_limits<double>::max() },
                     { std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest() }};
    for (const Vec3& corner : corners) {
        const Vec3 p = transform_point(matrix, corner);
        out.min.x = std::min(out.min.x, p.x);
        out.min.y = std::min(out.min.y, p.y);
        out.min.z = std::min(out.min.z, p.z);
        out.max.x = std::max(out.max.x, p.x);
        out.max.y = std::max(out.max.y, p.y);
        out.max.z = std::max(out.max.z, p.z);
    }
    return out;
}

enum class ShapeKind {
    Empty,
    Box,
    Cylinder,
    Sphere,
    Cone,
    Torus,
    Wire,
    Face,
    Extrusion,
    Revolve,
    Sweep,
    Loft,
    BooleanUnion,
    BooleanSubtract,
    BooleanIntersect,
    Transform,
    Fillet,
    Chamfer,
    Shell,
    Draft,
};

enum class SketchEntityType {
    Line,
    Arc,
    Circle,
    Spline,
};

struct SketchEntity {
    SketchEntityType type {SketchEntityType::Line};
    std::vector<Vec3> points;
    double radius {0.0};
    double start_angle {0.0};
    double end_angle {0.0};
};

struct ShapeData {
    ShapeKind kind {ShapeKind::Empty};
    std::string debug_name;
    std::string label;
    std::string reference_id;
    BoundingBox bbox {};
    double volume {0.0};
    double surface_area {0.0};
    bool valid {true};
    bool closed {true};
    std::vector<double> params;
    std::vector<std::shared_ptr<ShapeData>> children;
    Matrix4 transform {Matrix4::identity()};
    std::vector<Vec3> sampled_points;
    std::vector<std::string> history_tags;
    std::unordered_map<std::string, std::string> metadata;
#ifndef CAIDE_STUB_MODE
    TopoDS_Shape native_shape;
#endif
};

struct SketchData {
    std::vector<SketchEntity> entities;
    bool closed {false};
    BoundingBox bbox {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
    std::weak_ptr<struct DocumentData> owner;
};

struct MeshData {
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<uint32_t> indices;
};

struct ReferenceData {
    std::string id;
    std::string label;
    std::weak_ptr<ShapeData> shape;
    bool valid {true};
};

struct HistoryEntry {
    std::string id;
    std::string type;
    std::string params_json;
    std::string target_ref;
    std::shared_ptr<ShapeData> result;
};

struct DocumentData {
    std::mutex mutex;
    std::unordered_map<std::string, std::shared_ptr<ShapeData>> shapes_by_ref;
    std::vector<HistoryEntry> history;
    std::vector<HistoryEntry> redo_stack;
    int next_ref_id {1};
    std::string document_id {"doc-1"};
};

struct ContextData {
    std::mutex mutex;
    std::vector<std::weak_ptr<DocumentData>> documents;
};

struct Status {
    CaideError code {CAIDE_OK};
    std::string message {};

    explicit operator bool() const { return code == CAIDE_OK; }

    static Status ok() { return {}; }
    static Status error(CaideError c, std::string msg) { return {c, std::move(msg)}; }
};

template <typename T>
struct Result {
    Status status;
    T value {};

    explicit operator bool() const { return static_cast<bool>(status); }
};

inline bool nearly_equal(double a, double b, double epsilon = 1e-6) {
    return std::fabs(a - b) <= epsilon;
}

inline std::string format_double(double value, int precision = 3) {
    std::ostringstream stream;
    stream.setf(std::ios::fixed, std::ios::floatfield);
    stream.precision(precision);
    stream << value;
    return stream.str();
}

inline std::string next_reference_id(DocumentData& doc, const std::string& prefix) {
    std::ostringstream stream;
    stream << prefix << '-' << doc.next_ref_id++;
    return stream.str();
}

inline std::shared_ptr<ShapeData> clone_shape_data(const std::shared_ptr<ShapeData>& shape) {
    auto clone = std::make_shared<ShapeData>(*shape);
    clone->children = shape->children;
    clone->sampled_points = shape->sampled_points;
    return clone;
}

inline void assign_reference(DocumentData& doc, const std::shared_ptr<ShapeData>& shape, const std::string& prefix) {
    if (shape->reference_id.empty()) {
        shape->reference_id = next_reference_id(doc, prefix);
    }
    doc.shapes_by_ref[shape->reference_id] = shape;
}

inline std::vector<Vec3> bbox_corners(const BoundingBox& bbox) {
    return {
        {bbox.min.x, bbox.min.y, bbox.min.z}, {bbox.max.x, bbox.min.y, bbox.min.z},
        {bbox.max.x, bbox.max.y, bbox.min.z}, {bbox.min.x, bbox.max.y, bbox.min.z},
        {bbox.min.x, bbox.min.y, bbox.max.z}, {bbox.max.x, bbox.min.y, bbox.max.z},
        {bbox.max.x, bbox.max.y, bbox.max.z}, {bbox.min.x, bbox.max.y, bbox.max.z},
    };
}

inline std::string shape_kind_name(ShapeKind kind) {
    switch (kind) {
        case ShapeKind::Box: return "box";
        case ShapeKind::Cylinder: return "cylinder";
        case ShapeKind::Sphere: return "sphere";
        case ShapeKind::Cone: return "cone";
        case ShapeKind::Torus: return "torus";
        case ShapeKind::Wire: return "wire";
        case ShapeKind::Face: return "face";
        case ShapeKind::Extrusion: return "extrusion";
        case ShapeKind::Revolve: return "revolve";
        case ShapeKind::Sweep: return "sweep";
        case ShapeKind::Loft: return "loft";
        case ShapeKind::BooleanUnion: return "boolean_union";
        case ShapeKind::BooleanSubtract: return "boolean_subtract";
        case ShapeKind::BooleanIntersect: return "boolean_intersect";
        case ShapeKind::Transform: return "transform";
        case ShapeKind::Fillet: return "fillet";
        case ShapeKind::Chamfer: return "chamfer";
        case ShapeKind::Shell: return "shell";
        case ShapeKind::Draft: return "draft";
        default: return "empty";
    }
}

inline Status validate_positive(const char* name, double value) {
    if (!std::isfinite(value) || value <= 0.0) {
        std::ostringstream stream;
        stream << name << " must be > 0";
        return Status::error(CAIDE_ERR_INVALID_PARAM, stream.str());
    }
    return Status::ok();
}

inline Status validate_handle(bool ok, const char* name) {
    if (!ok) {
        std::ostringstream stream;
        stream << name << " is null";
        return Status::error(CAIDE_ERR_NULL_HANDLE, stream.str());
    }
    return Status::ok();
}

Status set_error(CaideError code, const std::string& message);
void clear_error();

Result<std::shared_ptr<ShapeData>> require_shape(const std::shared_ptr<ShapeData>& shape, const char* name);
Result<std::shared_ptr<DocumentData>> require_document(const std::shared_ptr<DocumentData>& doc, const char* name);
Result<BoundingBox> require_valid_bbox(const BoundingBox& bbox, const char* label);

std::shared_ptr<ShapeData> make_shape(ShapeKind kind, const std::string& name, const BoundingBox& bbox, double volume, double area);
std::shared_ptr<MeshData> make_mesh();
Result<std::shared_ptr<ShapeData>> clone_with_transform(const std::shared_ptr<ShapeData>& shape, const Matrix4& matrix, const std::string& reason);
void push_history(DocumentData& doc, const std::string& type, const std::string& params_json, const std::string& target_ref, const std::shared_ptr<ShapeData>& shape);
Result<std::shared_ptr<ShapeData>> resolve_reference(DocumentData& doc, const std::string& ref_id);
std::string trim_copy(const std::string& value);
std::string lowercase_copy(const std::string& value);
std::vector<std::string> split_csv(const std::string& value);

}  // namespace caide

struct CaideContext_t {
    std::shared_ptr<caide::ContextData> impl;
};

struct CaideDocument_t {
    std::shared_ptr<caide::DocumentData> impl;
};

struct CaideShape_t {
    std::shared_ptr<caide::ShapeData> impl;
};

struct CaideSketch_t {
    std::shared_ptr<caide::SketchData> impl;
};

struct CaideMesh_t {
    std::shared_ptr<caide::MeshData> impl;
};

struct CaideReference_t {
    std::shared_ptr<caide::ReferenceData> impl;
};

namespace caide {

inline CaideShape make_shape_handle(const std::shared_ptr<ShapeData>& shape) {
    auto* handle = new CaideShape_t();
    handle->impl = shape;
    return handle;
}

inline CaideSketch make_sketch_handle(const std::shared_ptr<SketchData>& sketch) {
    auto* handle = new CaideSketch_t();
    handle->impl = sketch;
    return handle;
}

inline CaideMesh make_mesh_handle(const std::shared_ptr<MeshData>& mesh) {
    auto* handle = new CaideMesh_t();
    handle->impl = mesh;
    return handle;
}

inline CaideReference make_reference_handle(const std::shared_ptr<ReferenceData>& reference) {
    auto* handle = new CaideReference_t();
    handle->impl = reference;
    return handle;
}

}  // namespace caide
