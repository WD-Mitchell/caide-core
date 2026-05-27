#include "sketch.h"

#ifndef CAIDE_STUB_MODE
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <GC_MakeSegment.hxx>
#include <GeomAPI_PointsToBSpline.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <TopoDS.hxx>
#endif

namespace caide::geometry {
namespace {

void expand_bbox(BoundingBox& bbox, double x, double y) {
    bbox.min.x = std::min(bbox.min.x, x);
    bbox.min.y = std::min(bbox.min.y, y);
    bbox.max.x = std::max(bbox.max.x, x);
    bbox.max.y = std::max(bbox.max.y, y);
}

std::vector<Vec3> sample_arc(double cx, double cy, double radius, double start_deg, double end_deg, int steps) {
    std::vector<Vec3> samples;
    const double delta = (end_deg - start_deg) / static_cast<double>(steps);
    for (int i = 0; i <= steps; ++i) {
        const double angle = (start_deg + (delta * i)) * kPi / 180.0;
        samples.push_back({cx + (std::cos(angle) * radius), cy + (std::sin(angle) * radius), 0.0});
    }
    return samples;
}

std::vector<Vec3> sketch_points(const SketchData& sketch) {
    std::vector<Vec3> points;
    for (const SketchEntity& entity : sketch.entities) {
        if (entity.type == SketchEntityType::Line || entity.type == SketchEntityType::Spline) {
            points.insert(points.end(), entity.points.begin(), entity.points.end());
        } else if (entity.type == SketchEntityType::Arc || entity.type == SketchEntityType::Circle) {
            auto samples = sample_arc(entity.points.front().x, entity.points.front().y, entity.radius, entity.start_angle, entity.end_angle, entity.type == SketchEntityType::Circle ? 24 : 12);
            points.insert(points.end(), samples.begin(), samples.end());
        }
    }
    return points;
}

double polygon_area(const std::vector<Vec3>& points) {
    if (points.size() < 3) {
        return 0.0;
    }
    double area = 0.0;
    for (std::size_t index = 0; index < points.size(); ++index) {
        const Vec3& current = points[index];
        const Vec3& next = points[(index + 1U) % points.size()];
        area += (current.x * next.y) - (next.x * current.y);
    }
    return std::fabs(area) * 0.5;
}

}  // namespace

Result<std::shared_ptr<SketchData>> create_sketch(const std::shared_ptr<DocumentData>& doc) {
    auto owner = require_document(doc, "document");
    if (!owner) {
        return {owner.status, nullptr};
    }
    auto sketch = std::make_shared<SketchData>();
    sketch->owner = doc;
    sketch->bbox = {{std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), 0.0},
                    {std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest(), 0.0}};
    clear_error();
    return {Status::ok(), sketch};
}

Status add_line(SketchData& sketch, double x1, double y1, double x2, double y2) {
    if (nearly_equal(x1, x2) && nearly_equal(y1, y2)) {
        return set_error(CAIDE_ERR_INVALID_PARAM, "line requires two distinct points");
    }
    SketchEntity entity;
    entity.type = SketchEntityType::Line;
    entity.points = {{x1, y1, 0.0}, {x2, y2, 0.0}};
    sketch.entities.push_back(entity);
    expand_bbox(sketch.bbox, x1, y1);
    expand_bbox(sketch.bbox, x2, y2);
    clear_error();
    return Status::ok();
}

Status add_arc(SketchData& sketch, double cx, double cy, double radius, double start_deg, double end_deg) {
    if (auto status = validate_positive("radius", radius); !status) {
        return set_error(status.code, status.message);
    }
    SketchEntity entity;
    entity.type = SketchEntityType::Arc;
    entity.points = {{cx, cy, 0.0}};
    entity.radius = radius;
    entity.start_angle = start_deg;
    entity.end_angle = end_deg;
    sketch.entities.push_back(entity);
    for (const Vec3& sample : sample_arc(cx, cy, radius, start_deg, end_deg, 16)) {
        expand_bbox(sketch.bbox, sample.x, sample.y);
    }
    clear_error();
    return Status::ok();
}

Status add_circle(SketchData& sketch, double cx, double cy, double radius) {
    if (auto status = validate_positive("radius", radius); !status) {
        return set_error(status.code, status.message);
    }
    SketchEntity entity;
    entity.type = SketchEntityType::Circle;
    entity.points = {{cx, cy, 0.0}};
    entity.radius = radius;
    entity.start_angle = 0.0;
    entity.end_angle = 360.0;
    sketch.entities.push_back(entity);
    expand_bbox(sketch.bbox, cx - radius, cy - radius);
    expand_bbox(sketch.bbox, cx + radius, cy + radius);
    clear_error();
    return Status::ok();
}

Status add_spline(SketchData& sketch, const double* points, int point_count) {
    if (points == nullptr) {
        return set_error(CAIDE_ERR_INVALID_PARAM, "points cannot be null");
    }
    if (point_count < 4 || (point_count % 2) != 0) {
        return set_error(CAIDE_ERR_INVALID_PARAM, "point_count must contain at least two XY pairs");
    }
    SketchEntity entity;
    entity.type = SketchEntityType::Spline;
    for (int index = 0; index < point_count; index += 2) {
        const double x = points[index];
        const double y = points[index + 1];
        entity.points.push_back({x, y, 0.0});
        expand_bbox(sketch.bbox, x, y);
    }
    sketch.entities.push_back(entity);
    clear_error();
    return Status::ok();
}

Status close_sketch(SketchData& sketch) {
    if (sketch.entities.empty()) {
        return set_error(CAIDE_ERR_OPERATION_FAILED, "cannot close an empty sketch");
    }
    sketch.closed = true;
    clear_error();
    return Status::ok();
}

Result<std::shared_ptr<ShapeData>> sketch_to_wire(const SketchData& sketch) {
    if (sketch.entities.empty()) {
        return {set_error(CAIDE_ERR_OPERATION_FAILED, "sketch has no entities"), nullptr};
    }
    const auto points = sketch_points(sketch);
    if (points.size() < 2) {
        return {set_error(CAIDE_ERR_OPERATION_FAILED, "sketch does not contain enough points to create a wire"), nullptr};
    }
    auto wire = make_shape(ShapeKind::Wire, "wire", make_bbox(sketch.bbox.min, sketch.bbox.max), 0.0, 0.0);
    wire->closed = sketch.closed;
    wire->sampled_points = points;
    wire->metadata["entity_count"] = std::to_string(sketch.entities.size());
#ifndef CAIDE_STUB_MODE
    BRepBuilderAPI_MakeWire wire_builder;
    for (const SketchEntity& entity : sketch.entities) {
        if (entity.type == SketchEntityType::Line) {
            wire_builder.Add(BRepBuilderAPI_MakeEdge(GC_MakeSegment(gp_Pnt(entity.points[0].x, entity.points[0].y, 0.0), gp_Pnt(entity.points[1].x, entity.points[1].y, 0.0))));
        } else if (entity.type == SketchEntityType::Arc || entity.type == SketchEntityType::Circle) {
            auto samples = sample_arc(entity.points.front().x, entity.points.front().y, entity.radius, entity.start_angle, entity.end_angle, entity.type == SketchEntityType::Circle ? 24 : 16);
            for (std::size_t i = 1; i < samples.size(); ++i) {
                wire_builder.Add(BRepBuilderAPI_MakeEdge(GC_MakeSegment(gp_Pnt(samples[i - 1].x, samples[i - 1].y, 0.0), gp_Pnt(samples[i].x, samples[i].y, 0.0))));
            }
        } else if (entity.type == SketchEntityType::Spline) {
            TColgp_Array1OfPnt array(1, static_cast<Standard_Integer>(entity.points.size()));
            for (Standard_Integer index = 1; index <= array.Length(); ++index) {
                const Vec3& point = entity.points[static_cast<std::size_t>(index - 1)];
                array.SetValue(index, gp_Pnt(point.x, point.y, 0.0));
            }
            Handle(Geom_BSplineCurve) spline = GeomAPI_PointsToBSpline(array).Curve();
            wire_builder.Add(BRepBuilderAPI_MakeEdge(spline));
        }
    }
    if (!wire_builder.IsDone()) {
        return {set_error(CAIDE_ERR_OPERATION_FAILED, "OCCT wire construction failed"), nullptr};
    }
    wire->native_shape = wire_builder.Wire();
#endif
    clear_error();
    return {Status::ok(), wire};
}

Result<std::shared_ptr<ShapeData>> sketch_to_face(const SketchData& sketch) {
    if (!sketch.closed && !(sketch.entities.size() == 1 && sketch.entities.front().type == SketchEntityType::Circle)) {
        return {set_error(CAIDE_ERR_OPERATION_FAILED, "sketch must be closed before creating a face"), nullptr};
    }
    auto wire_result = sketch_to_wire(sketch);
    if (!wire_result) {
        return {wire_result.status, nullptr};
    }
    const auto points = sketch_points(sketch);
    const double area = (sketch.entities.size() == 1 && sketch.entities.front().type == SketchEntityType::Circle)
        ? (kPi * sketch.entities.front().radius * sketch.entities.front().radius)
        : polygon_area(points);
    auto face = make_shape(ShapeKind::Face, "face", make_bbox(sketch.bbox.min, sketch.bbox.max), 0.0, area);
    face->sampled_points = points;
    face->closed = true;
    face->metadata["planar_area"] = format_double(area, 6);
#ifndef CAIDE_STUB_MODE
    BRepBuilderAPI_MakeFace face_builder(TopoDS::Wire(wire_result.value->native_shape));
    if (!face_builder.IsDone()) {
        return {set_error(CAIDE_ERR_OPERATION_FAILED, "OCCT face construction failed"), nullptr};
    }
    face->native_shape = face_builder.Face();
#endif
    clear_error();
    return {Status::ok(), face};
}

}  // namespace caide::geometry
