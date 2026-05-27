#pragma once

#include "../internal.h"

namespace caide::geometry {

Result<std::shared_ptr<ShapeData>> create_box(DocumentData& doc, double width, double height, double depth);
Result<std::shared_ptr<ShapeData>> create_cylinder(DocumentData& doc, double radius, double height);
Result<std::shared_ptr<ShapeData>> create_sphere(DocumentData& doc, double radius);
Result<std::shared_ptr<ShapeData>> create_cone(DocumentData& doc, double radius1, double radius2, double height);
Result<std::shared_ptr<ShapeData>> create_torus(DocumentData& doc, double major_radius, double minor_radius);

}  // namespace caide::geometry
