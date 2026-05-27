#include "../printability/analyzer.h"
#include "../internal.h"

extern "C" {

CaideError caide_print_analyze(CaideShape shape, const CaidePrintAnalysisParams* params, CaidePrintIssue** out_issues, int* out_issue_count) {
    if (shape == nullptr || shape->impl == nullptr) {
        return caide::set_error(CAIDE_ERR_NULL_HANDLE, "shape is null").code;
    }
    if (out_issues == nullptr || out_issue_count == nullptr) {
        return caide::set_error(CAIDE_ERR_INVALID_PARAM, "out_issues and out_issue_count are required").code;
    }
    auto result = caide::printability::analyze(shape->impl, params);
    if (!result) {
        return result.status.code;
    }
    *out_issue_count = static_cast<int>(result.value.size());
    if (result.value.empty()) {
        *out_issues = nullptr;
        caide::clear_error();
        return CAIDE_OK;
    }
    auto* issues = static_cast<CaidePrintIssue*>(std::malloc(sizeof(CaidePrintIssue) * result.value.size()));
    if (issues == nullptr) {
        return caide::set_error(CAIDE_ERR_OUT_OF_MEMORY, "failed to allocate issue array").code;
    }
    for (std::size_t i = 0; i < result.value.size(); ++i) {
        issues[i] = result.value[i];
    }
    *out_issues = issues;
    caide::clear_error();
    return CAIDE_OK;
}

void caide_print_issues_free(CaidePrintIssue* issues, int count) {
    if (issues != nullptr) {
        for (int i = 0; i < count; ++i) {
            std::free(const_cast<char*>(issues[i].reason));
        }
        std::free(issues);
    }
    caide::clear_error();
}

CaideError caide_print_wall_thickness(CaideShape shape, double* out_min_thickness) {
    if (shape == nullptr || shape->impl == nullptr) {
        return caide::set_error(CAIDE_ERR_NULL_HANDLE, "shape is null").code;
    }
    if (out_min_thickness == nullptr) {
        return caide::set_error(CAIDE_ERR_INVALID_PARAM, "out_min_thickness is required").code;
    }
    auto result = caide::printability::minimum_wall_thickness(shape->impl);
    if (!result) {
        return result.status.code;
    }
    *out_min_thickness = result.value;
    return CAIDE_OK;
}

CaideError caide_print_support_volume(CaideShape shape, const CaidePrintAnalysisParams* params, double* out_volume) {
    if (shape == nullptr || shape->impl == nullptr) {
        return caide::set_error(CAIDE_ERR_NULL_HANDLE, "shape is null").code;
    }
    if (out_volume == nullptr) {
        return caide::set_error(CAIDE_ERR_INVALID_PARAM, "out_volume is required").code;
    }
    auto result = caide::printability::support_volume(shape->impl, params);
    if (!result) {
        return result.status.code;
    }
    *out_volume = result.value;
    return CAIDE_OK;
}

CaideError caide_print_bed_adhesion_area(CaideShape shape, double* out_area) {
    if (shape == nullptr || shape->impl == nullptr) {
        return caide::set_error(CAIDE_ERR_NULL_HANDLE, "shape is null").code;
    }
    if (out_area == nullptr) {
        return caide::set_error(CAIDE_ERR_INVALID_PARAM, "out_area is required").code;
    }
    auto result = caide::printability::bed_adhesion_area(shape->impl);
    if (!result) {
        return result.status.code;
    }
    *out_area = result.value;
    return CAIDE_OK;
}

}  // extern "C"
