#pragma once

#include "../internal.h"

namespace caide::geometry {

Result<std::shared_ptr<ShapeData>> boolean_union(const std::shared_ptr<ShapeData>& a, const std::shared_ptr<ShapeData>& b);
Result<std::shared_ptr<ShapeData>> boolean_subtract(const std::shared_ptr<ShapeData>& a, const std::shared_ptr<ShapeData>& b);
Result<std::shared_ptr<ShapeData>> boolean_intersect(const std::shared_ptr<ShapeData>& a, const std::shared_ptr<ShapeData>& b);

}  // namespace caide::geometry
