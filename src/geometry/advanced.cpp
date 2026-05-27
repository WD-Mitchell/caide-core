#include "advanced.h"

#ifndef CAIDE_STUB_MODE
#include <BRepOffsetAPI_MakePipe.hxx>
#include <BRepOffsetAPI_ThruSections.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>
#include <gp_Ax1.hxx>
#include <gp_Dir.hxx>
#include <gp_Vec.hxx>
#endif

namespace caide::geometry {
namespace {

double profile_area(const ShapeData& shape) {
    if (shape.kind == ShapeKind::Face) {
        return shape.surface_area;
    }
    const auto size = bbox_size(shape.bbox);
    return std::max(size.x * size.y, shape.surface_area * 0.25);
}

double spine_length(const ShapeData& shape) {
    if (shape.sampled_points.size() > 1) {
        double total = 0.0;
        for (std::size_t i = 1; i < shape.sampled_points.size(); ++i) {
            total += length(shape.sampled_points[i] - shape.sampled_points[i - 1]);
        }
        return total;
    }
    const auto size = bbox_size(shape.bbox);
    return std::max({size.x, size.y, size.z});
}

}  // namespace

Result<std::shared_ptr<ShapeData>> extrude(const std::shared_ptr<ShapeData>& profile, double dx, double dy, double dz) {
    auto required = require_shape(profile, "profile");
    if (!required) {
        return {required.status, nullptr};
    }
    if (nearly_equal(length({dx, dy, dz}), 0.0)) {
        return {set_error(CAIDE_ERR_INVALID_PARAM, "extrusion vector cannot be zero"), nullptr};
    }

    BoundingBox bbox = union_bbox(profile->bbox, transform_bbox(profile->bbox, Matrix4::translation({dx, dy, dz})));
    const double distance = length({dx, dy, dz});
    const double area = profile_area(*profile);
    auto shape = make_shape(ShapeKind::Extrusion, "extrude", bbox, area * distance, (2.0 * area) + (distance * std::sqrt(std::max(area, 0.0)) * 4.0));
    shape->children = {profile};
    shape->params = {dx, dy, dz};
    shape->metadata["operation"] = "extrude";
#ifndef CAIDE_STUB_MODE
    shape->native_shape = BRepPrimAPI_MakePrism(profile->native_shape, gp_Vec(dx, dy, dz)).Shape();
#endif
    clear_error();
    return {Status::ok(), shape};
}

Result<std::shared_ptr<ShapeData>> revolve(const std::shared_ptr<ShapeData>& profile, double axis_x, double axis_y, double axis_z, double angle_deg) {
    auto required = require_shape(profile, "profile");
    if (!required) {
        return {required.status, nullptr};
    }
    if (nearly_equal(length({axis_x, axis_y, axis_z}), 0.0)) {
        return {set_error(CAIDE_ERR_INVALID_PARAM, "revolve axis cannot be zero"), nullptr};
    }

    const Vec3 center = bbox_center(profile->bbox);
    const double radius = std::max(length(center), 1.0);
    const double angle_ratio = std::fabs(angle_deg) / 360.0;
    const double area = profile_area(*profile);
    const double volume = area * (2.0 * kPi * radius) * angle_ratio;
    BoundingBox bbox = profile->bbox;
    const auto size = bbox_size(profile->bbox);
    bbox.max.x = std::max(bbox.max.x, radius + size.x);
    bbox.max.y = std::max(bbox.max.y, radius + size.y);
    bbox.min.x = std::min(bbox.min.x, -radius - size.x);
    bbox.min.y = std::min(bbox.min.y, -radius - size.y);
    auto shape = make_shape(ShapeKind::Revolve, "revolve", bbox, volume, std::max(profile->surface_area * std::max(angle_ratio, 0.1), 0.0));
    shape->children = {profile};
    shape->params = {axis_x, axis_y, axis_z, angle_deg};
    shape->metadata["operation"] = "revolve";
#ifndef CAIDE_STUB_MODE
    shape->native_shape = BRepPrimAPI_MakeRevol(profile->native_shape, gp_Ax1(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(axis_x, axis_y, axis_z)), angle_deg * kPi / 180.0).Shape();
#endif
    clear_error();
    return {Status::ok(), shape};
}

Result<std::shared_ptr<ShapeData>> sweep(const std::shared_ptr<ShapeData>& profile, const std::shared_ptr<ShapeData>& spine) {
    auto profile_required = require_shape(profile, "profile");
    if (!profile_required) {
        return {profile_required.status, nullptr};
    }
    auto spine_required = require_shape(spine, "spine");
    if (!spine_required) {
        return {spine_required.status, nullptr};
    }

    const double path_length = std::max(spine_length(*spine), 1.0);
    const double area = profile_area(*profile);
    BoundingBox bbox = union_bbox(profile->bbox, spine->bbox);
    const auto profile_size = bbox_size(profile->bbox);
    bbox.max.x += profile_size.x * 0.5;
    bbox.max.y += profile_size.y * 0.5;
    bbox.max.z += profile_size.z * 0.5;
    auto shape = make_shape(ShapeKind::Sweep, "sweep", bbox, area * path_length, profile->surface_area + (path_length * 2.0 * std::sqrt(std::max(area, 0.0))));
    shape->children = {profile, spine};
    shape->params = {path_length};
    shape->metadata["operation"] = "sweep";
#ifndef CAIDE_STUB_MODE
    if (spine->kind != ShapeKind::Wire) {
        return {set_error(CAIDE_ERR_INVALID_PARAM, "OCCT sweep requires a wire spine"), nullptr};
    }
    BRepOffsetAPI_MakePipe pipe(TopoDS::Wire(spine->native_shape), profile->native_shape);
    pipe.Build();
    if (!pipe.IsDone()) {
        return {set_error(CAIDE_ERR_OPERATION_FAILED, "OCCT sweep failed"), nullptr};
    }
    shape->native_shape = pipe.Shape();
#endif
    clear_error();
    return {Status::ok(), shape};
}

Result<std::shared_ptr<ShapeData>> loft(const std::vector<std::shared_ptr<ShapeData>>& profiles) {
    if (profiles.size() < 2) {
        return {set_error(CAIDE_ERR_INVALID_PARAM, "loft requires at least two profiles"), nullptr};
    }

    BoundingBox bbox = profiles.front()->bbox;
    double area_sum = 0.0;
    for (const auto& profile : profiles) {
        auto required = require_shape(profile, "profile");
        if (!required) {
            return {required.status, nullptr};
        }
        bbox = union_bbox(bbox, profile->bbox);
        area_sum += profile_area(*profile);
    }
    const auto size = bbox_size(bbox);
    const double path_hint = std::max({size.x, size.y, size.z, 1.0});
    auto shape = make_shape(ShapeKind::Loft, "loft", bbox, (area_sum / static_cast<double>(profiles.size())) * path_hint, bbox_surface_area(bbox));
    shape->children = profiles;
    shape->params = {static_cast<double>(profiles.size())};
    shape->metadata["operation"] = "loft";
#ifndef CAIDE_STUB_MODE
    BRepOffsetAPI_ThruSections loft_builder(true, false, 1.0e-3);
    for (const auto& profile : profiles) {
        if (profile->kind == ShapeKind::Wire) {
            loft_builder.AddWire(TopoDS::Wire(profile->native_shape));
        } else {
            return {set_error(CAIDE_ERR_INVALID_PARAM, "OCCT loft requires wire profiles"), nullptr};
        }
    }
    loft_builder.Build();
    if (!loft_builder.IsDone()) {
        return {set_error(CAIDE_ERR_OPERATION_FAILED, "OCCT loft failed"), nullptr};
    }
    shape->native_shape = loft_builder.Shape();
#endif
    clear_error();
    return {Status::ok(), shape};
}

}  // namespace caide::geometry
