#include "mesh_export.h"

namespace caide::exporter {
namespace {

void add_vertex(MeshData& mesh, const Vec3& position, const Vec3& normal) {
    mesh.vertices.push_back(static_cast<float>(position.x));
    mesh.vertices.push_back(static_cast<float>(position.y));
    mesh.vertices.push_back(static_cast<float>(position.z));
    mesh.normals.push_back(static_cast<float>(normal.x));
    mesh.normals.push_back(static_cast<float>(normal.y));
    mesh.normals.push_back(static_cast<float>(normal.z));
}

void add_triangle(MeshData& mesh, const Vec3& a, const Vec3& b, const Vec3& c) {
    const Vec3 normal = normalized(cross(b - a, c - a));
    const uint32_t base = static_cast<uint32_t>(mesh.vertices.size() / 3U);
    add_vertex(mesh, a, normal);
    add_vertex(mesh, b, normal);
    add_vertex(mesh, c, normal);
    mesh.indices.push_back(base);
    mesh.indices.push_back(base + 1U);
    mesh.indices.push_back(base + 2U);
}

void add_quad(MeshData& mesh, const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d) {
    add_triangle(mesh, a, b, c);
    add_triangle(mesh, a, c, d);
}

void add_box(MeshData& mesh, const BoundingBox& bbox) {
    const auto corners = bbox_corners(bbox);
    add_quad(mesh, corners[0], corners[1], corners[2], corners[3]);
    add_quad(mesh, corners[4], corners[5], corners[6], corners[7]);
    add_quad(mesh, corners[0], corners[1], corners[5], corners[4]);
    add_quad(mesh, corners[1], corners[2], corners[6], corners[5]);
    add_quad(mesh, corners[2], corners[3], corners[7], corners[6]);
    add_quad(mesh, corners[3], corners[0], corners[4], corners[7]);
}

void add_cylinder(MeshData& mesh, double radius, double height, int segments) {
    const Vec3 top_center {0.0, 0.0, height};
    const Vec3 bottom_center {0.0, 0.0, 0.0};
    for (int i = 0; i < segments; ++i) {
        const double a0 = (2.0 * kPi * static_cast<double>(i)) / static_cast<double>(segments);
        const double a1 = (2.0 * kPi * static_cast<double>(i + 1)) / static_cast<double>(segments);
        const Vec3 p0 {std::cos(a0) * radius, std::sin(a0) * radius, 0.0};
        const Vec3 p1 {std::cos(a1) * radius, std::sin(a1) * radius, 0.0};
        const Vec3 p2 {p1.x, p1.y, height};
        const Vec3 p3 {p0.x, p0.y, height};
        add_quad(mesh, p0, p1, p2, p3);
        add_triangle(mesh, bottom_center, p1, p0);
        add_triangle(mesh, top_center, p3, p2);
    }
}

void add_cone(MeshData& mesh, double radius1, double radius2, double height, int segments) {
    const Vec3 top_center {0.0, 0.0, height};
    const Vec3 bottom_center {0.0, 0.0, 0.0};
    for (int i = 0; i < segments; ++i) {
        const double a0 = (2.0 * kPi * static_cast<double>(i)) / static_cast<double>(segments);
        const double a1 = (2.0 * kPi * static_cast<double>(i + 1)) / static_cast<double>(segments);
        const Vec3 p0 {std::cos(a0) * radius1, std::sin(a0) * radius1, 0.0};
        const Vec3 p1 {std::cos(a1) * radius1, std::sin(a1) * radius1, 0.0};
        const Vec3 p2 {std::cos(a1) * radius2, std::sin(a1) * radius2, height};
        const Vec3 p3 {std::cos(a0) * radius2, std::sin(a0) * radius2, height};
        add_quad(mesh, p0, p1, p2, p3);
        add_triangle(mesh, bottom_center, p1, p0);
        add_triangle(mesh, top_center, p3, p2);
    }
}

void add_sphere(MeshData& mesh, double radius, int rings, int segments) {
    for (int ring = 0; ring < rings; ++ring) {
        const double t0 = kPi * static_cast<double>(ring) / static_cast<double>(rings);
        const double t1 = kPi * static_cast<double>(ring + 1) / static_cast<double>(rings);
        const double z0 = std::cos(t0) * radius;
        const double z1 = std::cos(t1) * radius;
        const double r0 = std::sin(t0) * radius;
        const double r1 = std::sin(t1) * radius;
        for (int seg = 0; seg < segments; ++seg) {
            const double a0 = (2.0 * kPi * static_cast<double>(seg)) / static_cast<double>(segments);
            const double a1 = (2.0 * kPi * static_cast<double>(seg + 1)) / static_cast<double>(segments);
            const Vec3 p0 {std::cos(a0) * r0, std::sin(a0) * r0, z0};
            const Vec3 p1 {std::cos(a1) * r0, std::sin(a1) * r0, z0};
            const Vec3 p2 {std::cos(a1) * r1, std::sin(a1) * r1, z1};
            const Vec3 p3 {std::cos(a0) * r1, std::sin(a0) * r1, z1};
            add_quad(mesh, p0, p1, p2, p3);
        }
    }
}

void add_torus(MeshData& mesh, double major_radius, double minor_radius, int major_segments, int minor_segments) {
    for (int major = 0; major < major_segments; ++major) {
        const double u0 = (2.0 * kPi * static_cast<double>(major)) / static_cast<double>(major_segments);
        const double u1 = (2.0 * kPi * static_cast<double>(major + 1)) / static_cast<double>(major_segments);
        for (int minor = 0; minor < minor_segments; ++minor) {
            const double v0 = (2.0 * kPi * static_cast<double>(minor)) / static_cast<double>(minor_segments);
            const double v1 = (2.0 * kPi * static_cast<double>(minor + 1)) / static_cast<double>(minor_segments);
            auto point = [&](double u, double v) -> Vec3 {
                const double ring = major_radius + (minor_radius * std::cos(v));
                return {std::cos(u) * ring, std::sin(u) * ring, minor_radius * std::sin(v)};
            };
            add_quad(mesh, point(u0, v0), point(u1, v0), point(u1, v1), point(u0, v1));
        }
    }
}

}  // namespace

Result<std::shared_ptr<MeshData>> tessellate(const std::shared_ptr<ShapeData>& shape, const CaideTessellationParams* params) {
    auto required = require_shape(shape, "shape");
    if (!required) {
        return {required.status, nullptr};
    }
    const CaideTessellationParams normalized = normalized_params(params);
    const int radial_segments = std::max(8, static_cast<int>(std::round(24.0 / std::max(normalized.linear_deflection, 0.05))));
    auto mesh = make_mesh();

    switch (shape->kind) {
        case ShapeKind::Box:
        case ShapeKind::Wire:
        case ShapeKind::Face:
        case ShapeKind::BooleanUnion:
        case ShapeKind::BooleanSubtract:
        case ShapeKind::BooleanIntersect:
        case ShapeKind::Transform:
        case ShapeKind::Fillet:
        case ShapeKind::Chamfer:
        case ShapeKind::Shell:
        case ShapeKind::Draft:
        case ShapeKind::Sweep:
        case ShapeKind::Loft:
            add_box(*mesh, shape->bbox);
            break;
        case ShapeKind::Cylinder:
            add_cylinder(*mesh, shape->params.empty() ? bbox_size(shape->bbox).x * 0.5 : shape->params[0], shape->params.size() > 1 ? shape->params[1] : bbox_size(shape->bbox).z, radial_segments);
            break;
        case ShapeKind::Cone:
            add_cone(*mesh,
                     shape->params.empty() ? bbox_size(shape->bbox).x * 0.5 : shape->params[0],
                     shape->params.size() > 1 ? shape->params[1] : bbox_size(shape->bbox).x * 0.25,
                     shape->params.size() > 2 ? shape->params[2] : bbox_size(shape->bbox).z,
                     radial_segments);
            break;
        case ShapeKind::Sphere:
            add_sphere(*mesh, shape->params.empty() ? bbox_size(shape->bbox).x * 0.5 : shape->params[0], radial_segments / 2, radial_segments);
            break;
        case ShapeKind::Torus:
            add_torus(*mesh,
                      shape->params.empty() ? bbox_size(shape->bbox).x * 0.25 : shape->params[0],
                      shape->params.size() > 1 ? shape->params[1] : bbox_size(shape->bbox).z * 0.5,
                      radial_segments,
                      radial_segments / 2);
            break;
        case ShapeKind::Extrusion:
        case ShapeKind::Revolve:
            add_box(*mesh, shape->bbox);
            break;
        default:
            add_box(*mesh, shape->bbox);
            break;
    }

    if (mesh->indices.empty()) {
        return {set_error(CAIDE_ERR_EXPORT_FAILED, "tessellation produced no triangles"), nullptr};
    }
    clear_error();
    return {Status::ok(), mesh};
}

}  // namespace caide::exporter
