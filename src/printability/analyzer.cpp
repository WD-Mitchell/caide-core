#include "analyzer.h"

#include <cstdio>
#include <cstdlib>

namespace caide::printability {

Result<double> estimate_support_volume_internal(const std::shared_ptr<ShapeData>& shape, const CaidePrintAnalysisParams& params);

namespace {

char* duplicate_reason(const std::string& value) {
    char* copy = static_cast<char*>(std::malloc(value.size() + 1U));
    if (copy == nullptr) {
        return nullptr;
    }
    std::memcpy(copy, value.c_str(), value.size() + 1U);
    return copy;
}

CaidePrintIssue make_issue(int face_index, CaidePrintSeverity severity, const std::string& reason, double value) {
    CaidePrintIssue issue {};
    issue.face_index = face_index;
    issue.severity = severity;
    issue.reason = duplicate_reason(reason);
    issue.value = value;
    return issue;
}

}  // namespace

CaidePrintAnalysisParams normalized_params(const CaidePrintAnalysisParams* params) {
    CaidePrintAnalysisParams out {};
    out.overhang_threshold_deg = params != nullptr && params->overhang_threshold_deg > 0.0 ? params->overhang_threshold_deg : 45.0;
    out.min_wall_thickness_mm = params != nullptr && params->min_wall_thickness_mm > 0.0 ? params->min_wall_thickness_mm : 0.4;
    out.min_feature_size_mm = params != nullptr && params->min_feature_size_mm > 0.0 ? params->min_feature_size_mm : 0.2;
    out.bridge_max_length_mm = params != nullptr && params->bridge_max_length_mm > 0.0 ? params->bridge_max_length_mm : 10.0;
    out.build_dir_x = params != nullptr ? params->build_dir_x : 0.0;
    out.build_dir_y = params != nullptr ? params->build_dir_y : 0.0;
    out.build_dir_z = params != nullptr && !nearly_equal(length({params->build_dir_x, params->build_dir_y, params->build_dir_z}), 0.0) ? params->build_dir_z : 1.0;
    return out;
}

Result<double> minimum_wall_thickness(const std::shared_ptr<ShapeData>& shape) {
    auto required = require_shape(shape, "shape");
    if (!required) {
        return {required.status, 0.0};
    }
    const auto size = bbox_size(shape->bbox);
    double thickness = std::min({std::fabs(size.x), std::fabs(size.y), std::fabs(size.z)});
    if (shape->kind == ShapeKind::Cylinder && !shape->params.empty()) {
        thickness = std::min(thickness, shape->params[0] * 2.0);
    }
    if (shape->kind == ShapeKind::Wire || shape->kind == ShapeKind::Face) {
        thickness = 0.0;
    }
    clear_error();
    return {Status::ok(), thickness};
}

Result<double> support_volume(const std::shared_ptr<ShapeData>& shape, const CaidePrintAnalysisParams* params) {
    return estimate_support_volume_internal(shape, normalized_params(params));
}

Result<double> bed_adhesion_area(const std::shared_ptr<ShapeData>& shape) {
    auto required = require_shape(shape, "shape");
    if (!required) {
        return {required.status, 0.0};
    }
    const auto size = bbox_size(shape->bbox);
    double area = std::max(size.x * size.y, 0.0);
    if (shape->kind == ShapeKind::Cylinder && !shape->params.empty()) {
        area = kPi * shape->params[0] * shape->params[0];
    }
    clear_error();
    return {Status::ok(), area};
}

Result<std::vector<CaidePrintIssue>> analyze(const std::shared_ptr<ShapeData>& shape, const CaidePrintAnalysisParams* params) {
    auto required = require_shape(shape, "shape");
    if (!required) {
        return {required.status, {}};
    }
    const CaidePrintAnalysisParams cfg = normalized_params(params);
    std::vector<CaidePrintIssue> issues;

    auto min_thickness = minimum_wall_thickness(shape);
    if (!min_thickness) {
        return {min_thickness.status, {}};
    }
    if (min_thickness.value < cfg.min_wall_thickness_mm) {
        issues.push_back(make_issue(0, CAIDE_PRINT_CRITICAL, "wall thickness below minimum", min_thickness.value));
    } else if (min_thickness.value < (cfg.min_wall_thickness_mm * 1.5)) {
        issues.push_back(make_issue(0, CAIDE_PRINT_WARNING, "wall thickness near minimum", min_thickness.value));
    }

    const auto size = bbox_size(shape->bbox);
    const double horizontal_span = std::max(size.x, size.y);
    const double vertical_span = std::max(size.z, 0.001);
    const double inferred_overhang = std::min(89.0, 20.0 + (horizontal_span / vertical_span) * 15.0);
    if (inferred_overhang > cfg.overhang_threshold_deg) {
        const CaidePrintSeverity severity = inferred_overhang > (cfg.overhang_threshold_deg + 20.0) ? CAIDE_PRINT_CRITICAL : CAIDE_PRINT_WARNING;
        issues.push_back(make_issue(1, severity, "overhang exceeds threshold", inferred_overhang));
    }

    if (horizontal_span > cfg.bridge_max_length_mm && size.z < (cfg.min_wall_thickness_mm * 10.0)) {
        issues.push_back(make_issue(2, CAIDE_PRINT_WARNING, "bridge span may sag", horizontal_span));
    }

    const double min_feature = std::min({std::fabs(size.x), std::fabs(size.y), std::fabs(size.z)});
    if (min_feature < cfg.min_feature_size_mm) {
        issues.push_back(make_issue(3, CAIDE_PRINT_CRITICAL, "feature size below nozzle/material capability", min_feature));
    }

    auto support = support_volume(shape, &cfg);
    if (!support) {
        return {support.status, {}};
    }
    if (support.value > shape->volume * 0.25) {
        issues.push_back(make_issue(4, CAIDE_PRINT_WARNING, "significant support volume estimated", support.value));
    }

    clear_error();
    return {Status::ok(), issues};
}

}  // namespace caide::printability
