#pragma once

#include "../internal.h"

namespace caide::geometry {

Result<std::shared_ptr<ShapeData>> extrude(const std::shared_ptr<ShapeData>& profile, double dx, double dy, double dz);
Result<std::shared_ptr<ShapeData>> revolve(const std::shared_ptr<ShapeData>& profile, double axis_x, double axis_y, double axis_z, double angle_deg);
Result<std::shared_ptr<ShapeData>> sweep(const std::shared_ptr<ShapeData>& profile, const std::shared_ptr<ShapeData>& spine);
Result<std::shared_ptr<ShapeData>> loft(const std::vector<std::shared_ptr<ShapeData>>& profiles);

}  // namespace caide::geometry
