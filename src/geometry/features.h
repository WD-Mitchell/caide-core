#pragma once

#include "../internal.h"

namespace caide::geometry {

Result<std::shared_ptr<ShapeData>> fillet(const std::shared_ptr<ShapeData>& shape, double radius, const std::vector<int>& edges);
Result<std::shared_ptr<ShapeData>> chamfer(const std::shared_ptr<ShapeData>& shape, double distance, const std::vector<int>& edges);
Result<std::shared_ptr<ShapeData>> shell(const std::shared_ptr<ShapeData>& shape, double thickness, const std::vector<int>& faces);
Result<std::shared_ptr<ShapeData>> draft(const std::shared_ptr<ShapeData>& shape, double angle_deg, const Vec3& direction, const std::vector<int>& faces);

}  // namespace caide::geometry
