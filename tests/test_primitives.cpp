#include <cmath>
#include <iostream>

#include "../src/error.h"
#include "../src/geometry/primitives.h"

#define CHECK(expr) do { if (!(expr)) { std::cerr << "CHECK failed: " #expr << " line=" << __LINE__ << '\n'; return 1; } } while (0)

namespace {

bool approx(double lhs, double rhs, double epsilon = 1e-6) {
    return std::fabs(lhs - rhs) <= epsilon;
}

bool verify_bbox(const caide::BoundingBox& bbox, double x, double y, double z) {
    return approx(bbox.max.x - bbox.min.x, x)
        && approx(bbox.max.y - bbox.min.y, y)
        && approx(bbox.max.z - bbox.min.z, z);
}

}  // namespace

int main() {
    caide::DocumentData doc;

    auto box = caide::geometry::create_box(doc, 10.0, 20.0, 30.0);
    CHECK(box);
    CHECK(approx(box.value->volume, 6000.0));
    CHECK(box.value->reference_id == "shape-1");
    CHECK(verify_bbox(box.value->bbox, 10.0, 20.0, 30.0));

    auto cylinder = caide::geometry::create_cylinder(doc, 5.0, 12.0);
    CHECK(cylinder);
    CHECK(cylinder.value->bbox.max.z == 12.0);
    CHECK(cylinder.value->surface_area > 0.0);

    auto sphere = caide::geometry::create_sphere(doc, 6.0);
    CHECK(sphere);
    CHECK(approx(sphere.value->bbox.min.x, -6.0));
    CHECK(approx(sphere.value->bbox.max.z, 6.0));

    auto cone = caide::geometry::create_cone(doc, 6.0, 2.0, 15.0);
    CHECK(cone);
    CHECK(cone.value->volume > 0.0);
    CHECK(cone.value->bbox.max.z == 15.0);

    auto torus = caide::geometry::create_torus(doc, 10.0, 2.0);
    CHECK(torus);
    CHECK(torus.value->surface_area > 0.0);
    CHECK(torus.value->bbox.max.x == 12.0);

    auto bad_box = caide::geometry::create_box(doc, -1.0, 2.0, 3.0);
    CHECK(!bad_box);
    CHECK(caide::last_error_code() == CAIDE_ERR_INVALID_PARAM);

    auto bad_torus = caide::geometry::create_torus(doc, 2.0, 2.0);
    CHECK(!bad_torus);
    CHECK(caide::last_error_code() == CAIDE_ERR_INVALID_PARAM);

    CHECK(doc.history.size() == 5U);
    CHECK(doc.shapes_by_ref.size() == 5U);
    return 0;
}
