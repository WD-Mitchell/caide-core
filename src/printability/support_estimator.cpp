#include "analyzer.h"

namespace caide::printability {
namespace {

double estimate_support_ratio(const ShapeData& shape, const CaidePrintAnalysisParams& params) {
    const auto size = bbox_size(shape.bbox);
    const double horizontal = std::max(size.x, size.y);
    const double vertical = std::max(size.z, 0.001);
    const double overhang_signal = horizontal / vertical;
    const double threshold_ratio = std::max(params.overhang_threshold_deg / 45.0, 0.25);
    double ratio = (overhang_signal - threshold_ratio) * 0.2;
    if (shape.kind == ShapeKind::Torus || shape.kind == ShapeKind::Sweep) {
        ratio += 0.15;
    }
    if (shape.kind == ShapeKind::Sphere) {
        ratio += 0.1;
    }
    return std::clamp(ratio, 0.0, 0.85);
}

}  // namespace

Result<double> estimate_support_volume_internal(const std::shared_ptr<ShapeData>& shape, const CaidePrintAnalysisParams& params) {
    auto required = require_shape(shape, "shape");
    if (!required) {
        return {required.status, 0.0};
    }
    const double ratio = estimate_support_ratio(*shape, params);
    const auto size = bbox_size(shape->bbox);
    const double envelope_volume = std::max(size.x * size.y * size.z, 0.0);
    const double support = std::max((envelope_volume - shape->volume) * 0.15, shape->volume * ratio);
    clear_error();
    return {Status::ok(), support};
}

}  // namespace caide::printability
