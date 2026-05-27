#include "booleans.h"

#ifndef CAIDE_STUB_MODE
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepBndLib.hxx>
#include <BRepGProp.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#endif

namespace caide::geometry {
namespace {

Status validate_inputs(const std::shared_ptr<ShapeData>& a, const std::shared_ptr<ShapeData>& b) {
    auto left = require_shape(a, "shape_a");
    if (!left) {
        return left.status;
    }
    auto right = require_shape(b, "shape_b");
    if (!right) {
        return right.status;
    }
    clear_error();
    return Status::ok();
}

std::shared_ptr<ShapeData> make_boolean_shape(ShapeKind kind,
                                              const std::shared_ptr<ShapeData>& a,
                                              const std::shared_ptr<ShapeData>& b,
                                              const BoundingBox& bbox,
                                              double volume,
                                              double area,
                                              const char* op) {
    auto shape = make_shape(kind, op, bbox, volume, area);
    shape->children = {a, b};
    shape->metadata["operation"] = op;
    shape->params = {a->volume, b->volume, area};
    return shape;
}

#ifndef CAIDE_STUB_MODE
void populate_occ_metrics(ShapeData& shape) {
    if (shape.native_shape.IsNull()) {
        return;
    }
    GProp_GProps properties;
    BRepGProp::VolumeProperties(shape.native_shape, properties);
    shape.volume = properties.Mass();
    BRepGProp::SurfaceProperties(shape.native_shape, properties);
    shape.surface_area = properties.Mass();
    Bnd_Box bbox;
    BRepBndLib::Add(shape.native_shape, bbox);
    Standard_Real xmin = 0.0;
    Standard_Real ymin = 0.0;
    Standard_Real zmin = 0.0;
    Standard_Real xmax = 0.0;
    Standard_Real ymax = 0.0;
    Standard_Real zmax = 0.0;
    bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    shape.bbox = make_bbox({xmin, ymin, zmin}, {xmax, ymax, zmax});
}
#endif

}  // namespace

Result<std::shared_ptr<ShapeData>> boolean_union(const std::shared_ptr<ShapeData>& a, const std::shared_ptr<ShapeData>& b) {
    if (auto status = validate_inputs(a, b); !status) {
        return {set_error(status.code, status.message), nullptr};
    }

    const BoundingBox bbox = union_bbox(a->bbox, b->bbox);
    const BoundingBox overlap = intersect_bbox(a->bbox, b->bbox);
    const double overlap_volume = bbox_is_valid(overlap) ? bbox_volume(overlap) * 0.35 : 0.0;
    const double overlap_area = bbox_is_valid(overlap) ? bbox_surface_area(overlap) * 0.2 : 0.0;
    const double volume = std::max(a->volume + b->volume - overlap_volume, std::max(a->volume, b->volume));
    const double area = std::max(a->surface_area + b->surface_area - overlap_area, bbox_surface_area(bbox));
    auto shape = make_boolean_shape(ShapeKind::BooleanUnion, a, b, bbox, volume, area, "union");
#ifndef CAIDE_STUB_MODE
    shape->native_shape = BRepAlgoAPI_Fuse(a->native_shape, b->native_shape).Shape();
    populate_occ_metrics(*shape);
#endif
    clear_error();
    return {Status::ok(), shape};
}

Result<std::shared_ptr<ShapeData>> boolean_subtract(const std::shared_ptr<ShapeData>& a, const std::shared_ptr<ShapeData>& b) {
    if (auto status = validate_inputs(a, b); !status) {
        return {set_error(status.code, status.message), nullptr};
    }

    const BoundingBox overlap = intersect_bbox(a->bbox, b->bbox);
    const double overlap_volume = bbox_is_valid(overlap) ? bbox_volume(overlap) * 0.6 : 0.0;
    const double volume = std::max(a->volume - std::min(a->volume, std::max(overlap_volume, b->volume * 0.25)), 0.0);
    const double area = std::max(a->surface_area + (bbox_is_valid(overlap) ? bbox_surface_area(overlap) * 0.1 : 0.0), 0.0);
    auto shape = make_boolean_shape(ShapeKind::BooleanSubtract, a, b, a->bbox, volume, area, "subtract");
#ifndef CAIDE_STUB_MODE
    shape->native_shape = BRepAlgoAPI_Cut(a->native_shape, b->native_shape).Shape();
    populate_occ_metrics(*shape);
#endif
    clear_error();
    return {Status::ok(), shape};
}

Result<std::shared_ptr<ShapeData>> boolean_intersect(const std::shared_ptr<ShapeData>& a, const std::shared_ptr<ShapeData>& b) {
    if (auto status = validate_inputs(a, b); !status) {
        return {set_error(status.code, status.message), nullptr};
    }

    const BoundingBox bbox = intersect_bbox(a->bbox, b->bbox);
    if (!bbox_is_valid(bbox)) {
        return {set_error(CAIDE_ERR_TOPOLOGY_ERROR, "Shapes do not overlap"), nullptr};
    }
    const double volume = std::min({a->volume, b->volume, std::max(bbox_volume(bbox), 0.0)});
    const double area = std::min({a->surface_area, b->surface_area, std::max(bbox_surface_area(bbox), 0.0)});
    auto shape = make_boolean_shape(ShapeKind::BooleanIntersect, a, b, bbox, volume, area, "intersect");
#ifndef CAIDE_STUB_MODE
    shape->native_shape = BRepAlgoAPI_Common(a->native_shape, b->native_shape).Shape();
    populate_occ_metrics(*shape);
#endif
    clear_error();
    return {Status::ok(), shape};
}

}  // namespace caide::geometry
