#pragma once

#include "../internal.h"

namespace caide::printability {

CaidePrintAnalysisParams normalized_params(const CaidePrintAnalysisParams* params);
Result<std::vector<CaidePrintIssue>> analyze(const std::shared_ptr<ShapeData>& shape, const CaidePrintAnalysisParams* params);
Result<double> minimum_wall_thickness(const std::shared_ptr<ShapeData>& shape);
Result<double> support_volume(const std::shared_ptr<ShapeData>& shape, const CaidePrintAnalysisParams* params);
Result<double> bed_adhesion_area(const std::shared_ptr<ShapeData>& shape);

}  // namespace caide::printability
