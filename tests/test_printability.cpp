#include <iostream>

#include "../src/geometry/primitives.h"
#include "../src/printability/analyzer.h"

#define CHECK(expr) do { if (!(expr)) { std::cerr << "CHECK failed: " #expr << " line=" << __LINE__ << '\n'; return 1; } } while (0)

int main() {
    caide::DocumentData doc;
    auto thin_box = caide::geometry::create_box(doc, 0.2, 20.0, 20.0);
    CHECK(thin_box);

    CaidePrintAnalysisParams params {};
    params.min_wall_thickness_mm = 0.4;
    auto issues = caide::printability::analyze(thin_box.value, &params);
    CHECK(issues);
    CHECK(!issues.value.empty());

    auto thickness = caide::printability::minimum_wall_thickness(thin_box.value);
    CHECK(thickness);
    CHECK(thickness.value < 0.4);

    auto support = caide::printability::support_volume(thin_box.value, &params);
    CHECK(support);
    CHECK(support.value >= 0.0);

    auto bed = caide::printability::bed_adhesion_area(thin_box.value);
    CHECK(bed);
    CHECK(bed.value > 0.0);

    auto sphere = caide::geometry::create_sphere(doc, 5.0);
    CHECK(sphere);
    auto sphere_issues = caide::printability::analyze(sphere.value, &params);
    CHECK(sphere_issues);
    auto sphere_support = caide::printability::support_volume(sphere.value, &params);
    CHECK(sphere_support);
    CHECK(sphere_support.value >= 0.0);

    for (auto& issue : issues.value) {
        std::free(const_cast<char*>(issue.reason));
    }
    for (auto& issue : sphere_issues.value) {
        std::free(const_cast<char*>(issue.reason));
    }
    return 0;
}
