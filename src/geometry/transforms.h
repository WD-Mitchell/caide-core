#pragma once

#include "../internal.h"

namespace caide::geometry {

Result<std::shared_ptr<ShapeData>> translate(const std::shared_ptr<ShapeData>& shape, double dx, double dy, double dz);
Result<std::shared_ptr<ShapeData>> rotate(const std::shared_ptr<ShapeData>& shape, double axis_x, double axis_y, double axis_z, double angle_deg);
Result<std::shared_ptr<ShapeData>> scale(const std::shared_ptr<ShapeData>& shape, double cx, double cy, double cz, double factor);
Result<std::shared_ptr<ShapeData>> mirror(const std::shared_ptr<ShapeData>& shape, double plane_x, double plane_y, double plane_z, double plane_d);

}  // namespace caide::geometry
