#pragma once

#include "../internal.h"

namespace caide::geometry {

Result<std::shared_ptr<SketchData>> create_sketch(const std::shared_ptr<DocumentData>& doc);
Status add_line(SketchData& sketch, double x1, double y1, double x2, double y2);
Status add_arc(SketchData& sketch, double cx, double cy, double radius, double start_deg, double end_deg);
Status add_circle(SketchData& sketch, double cx, double cy, double radius);
Status add_spline(SketchData& sketch, const double* points, int point_count);
Status close_sketch(SketchData& sketch);
Result<std::shared_ptr<ShapeData>> sketch_to_wire(const SketchData& sketch);
Result<std::shared_ptr<ShapeData>> sketch_to_face(const SketchData& sketch);

}  // namespace caide::geometry
