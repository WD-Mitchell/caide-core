#include "features.h"

#ifndef CAIDE_STUB_MODE
#include <BRepFilletAPI_MakeChamfer.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepOffsetAPI_DraftAngle.hxx>
#include <BRepOffsetAPI_MakeThickSolid.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#endif

namespace caide::geometry {
namespace {

std::vector<int> normalize_selection(const std::vector<int>& values, int max_count) {
    std::vector<int> out;
    for (int value : values) {
        if (value >= 0 && value < max_count) {
            out.push_back(value);
        }
    }
    if (out.empty() && max_count > 0) {
        out.push_back(0);
    }
    return out;
}

std::shared_ptr<ShapeData> feature_clone(const std::shared_ptr<ShapeData>& shape,
                                         ShapeKind kind,
                                         const char* operation,
                                         double amount,
                                         double volume_scale,
                                         double area_scale) {
    auto clone = clone_shape_data(shape);
    clone->kind = kind;
    clone->debug_name = operation;
    clone->metadata["feature"] = operation;
    clone->params = {amount};
    clone->volume = std::max(0.0, shape->volume * volume_scale);
    clone->surface_area = std::max(0.0, shape->surface_area * area_scale);
    clone->history_tags.push_back(operation);
    return clone;
}

#ifndef CAIDE_STUB_MODE
TopTools_IndexedMapOfShape collect_shapes(const TopoDS_Shape& shape, TopAbs_ShapeEnum type) {
    TopTools_IndexedMapOfShape map;
    TopExp::MapShapes(shape, type, map);
    return map;
}
#endif

}  // namespace

Result<std::shared_ptr<ShapeData>> fillet(const std::shared_ptr<ShapeData>& shape, double radius, const std::vector<int>& edges) {
    auto required = require_shape(shape, "shape");
    if (!required) {
        return {required.status, nullptr};
    }
    if (auto status = validate_positive("radius", radius); !status) {
        return {set_error(status.code, status.message), nullptr};
    }

    auto clone = feature_clone(shape, ShapeKind::Fillet, "fillet", radius, 0.985, 0.995);
    clone->params.insert(clone->params.end(), edges.begin(), edges.end());
#ifndef CAIDE_STUB_MODE
    BRepFilletAPI_MakeFillet fillet_builder(shape->native_shape);
    const auto edge_map = collect_shapes(shape->native_shape, TopAbs_EDGE);
    for (int index : normalize_selection(edges, edge_map.Extent())) {
        fillet_builder.Add(radius, TopoDS::Edge(edge_map(index + 1)));
    }
    fillet_builder.Build();
    if (!fillet_builder.IsDone()) {
        return {set_error(CAIDE_ERR_OPERATION_FAILED, "OCCT fillet failed"), nullptr};
    }
    clone->native_shape = fillet_builder.Shape();
#endif
    clear_error();
    return {Status::ok(), clone};
}

Result<std::shared_ptr<ShapeData>> chamfer(const std::shared_ptr<ShapeData>& shape, double distance, const std::vector<int>& edges) {
    auto required = require_shape(shape, "shape");
    if (!required) {
        return {required.status, nullptr};
    }
    if (auto status = validate_positive("distance", distance); !status) {
        return {set_error(status.code, status.message), nullptr};
    }

    auto clone = feature_clone(shape, ShapeKind::Chamfer, "chamfer", distance, 0.98, 0.992);
    clone->params.insert(clone->params.end(), edges.begin(), edges.end());
#ifndef CAIDE_STUB_MODE
    BRepFilletAPI_MakeChamfer chamfer_builder(shape->native_shape);
    const auto edge_map = collect_shapes(shape->native_shape, TopAbs_EDGE);
    const auto face_map = collect_shapes(shape->native_shape, TopAbs_FACE);
    if (face_map.Extent() == 0) {
        return {set_error(CAIDE_ERR_TOPOLOGY_ERROR, "No faces available for chamfer"), nullptr};
    }
    const TopoDS_Face anchor_face = TopoDS::Face(face_map(1));
    for (int index : normalize_selection(edges, edge_map.Extent())) {
        chamfer_builder.Add(distance, TopoDS::Edge(edge_map(index + 1)), anchor_face);
    }
    chamfer_builder.Build();
    if (!chamfer_builder.IsDone()) {
        return {set_error(CAIDE_ERR_OPERATION_FAILED, "OCCT chamfer failed"), nullptr};
    }
    clone->native_shape = chamfer_builder.Shape();
#endif
    clear_error();
    return {Status::ok(), clone};
}

Result<std::shared_ptr<ShapeData>> shell(const std::shared_ptr<ShapeData>& shape, double thickness, const std::vector<int>& faces) {
    auto required = require_shape(shape, "shape");
    if (!required) {
        return {required.status, nullptr};
    }
    if (auto status = validate_positive("thickness", thickness); !status) {
        return {set_error(status.code, status.message), nullptr};
    }

    const auto size = bbox_size(shape->bbox);
    const double max_possible = std::min({size.x, size.y, size.z}) * 0.45;
    if (max_possible > 0.0 && thickness >= max_possible) {
        return {set_error(CAIDE_ERR_INVALID_PARAM, "shell thickness is larger than the current body can support"), nullptr};
    }

    auto clone = feature_clone(shape, ShapeKind::Shell, "shell", thickness, 0.7, 1.05);
    clone->params.insert(clone->params.end(), faces.begin(), faces.end());
    clone->volume = std::max(shape->volume - (shape->surface_area * thickness * 0.35), 0.0);
#ifndef CAIDE_STUB_MODE
    TopTools_ListOfShape faces_to_remove;
    const auto face_map = collect_shapes(shape->native_shape, TopAbs_FACE);
    for (int index : normalize_selection(faces, face_map.Extent())) {
        faces_to_remove.Append(face_map(index + 1));
    }
    BRepOffsetAPI_MakeThickSolid builder;
    builder.MakeThickSolidByJoin(shape->native_shape, faces_to_remove, -thickness, 1.0e-3);
    builder.Build();
    if (!builder.IsDone()) {
        return {set_error(CAIDE_ERR_OPERATION_FAILED, "OCCT shell failed"), nullptr};
    }
    clone->native_shape = builder.Shape();
#endif
    clear_error();
    return {Status::ok(), clone};
}

Result<std::shared_ptr<ShapeData>> draft(const std::shared_ptr<ShapeData>& shape, double angle_deg, const Vec3& direction, const std::vector<int>& faces) {
    auto required = require_shape(shape, "shape");
    if (!required) {
        return {required.status, nullptr};
    }
    if (nearly_equal(length(direction), 0.0)) {
        return {set_error(CAIDE_ERR_INVALID_PARAM, "draft direction cannot be zero"), nullptr};
    }

    auto clone = feature_clone(shape, ShapeKind::Draft, "draft", angle_deg, 1.0, 1.01);
    clone->params = {angle_deg, direction.x, direction.y, direction.z};
    clone->params.insert(clone->params.end(), faces.begin(), faces.end());
    const double scale_factor = 1.0 + (std::tan(angle_deg * kPi / 180.0) * 0.05);
    clone->bbox = transform_bbox(shape->bbox, Matrix4::scale(bbox_center(shape->bbox), scale_factor));
#ifndef CAIDE_STUB_MODE
    BRepOffsetAPI_DraftAngle builder(shape->native_shape);
    const auto face_map = collect_shapes(shape->native_shape, TopAbs_FACE);
    gp_Dir direction_gp(direction.x, direction.y, direction.z);
    gp_Pln neutral(gp_Pnt(0.0, 0.0, 0.0), direction_gp);
    for (int index : normalize_selection(faces, face_map.Extent())) {
        builder.Add(TopoDS::Face(face_map(index + 1)), direction_gp, angle_deg * kPi / 180.0, neutral);
    }
    builder.Build();
    if (!builder.IsDone()) {
        return {set_error(CAIDE_ERR_OPERATION_FAILED, "OCCT draft failed"), nullptr};
    }
    clone->native_shape = builder.Shape();
#endif
    clear_error();
    return {Status::ok(), clone};
}

}  // namespace caide::geometry
