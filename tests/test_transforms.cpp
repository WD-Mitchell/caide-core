#include <cmath>
#include <iostream>

#include "../src/error.h"
#include "../src/geometry/primitives.h"
#include "../src/geometry/transforms.h"

#define CHECK(expr) do { if (!(expr)) { std::cerr << "CHECK failed: " #expr << " line=" << __LINE__ << '\n'; return 1; } } while (0)

namespace {

bool approx(double lhs, double rhs, double epsilon = 1e-6) {
    return std::fabs(lhs - rhs) <= epsilon;
}

}  // namespace

int main() {
    caide::DocumentData doc;
    auto box = caide::geometry::create_box(doc, 5.0, 6.0, 7.0);
    CHECK(box);

    auto moved = caide::geometry::translate(box.value, 3.0, 4.0, 5.0);
    CHECK(moved);
    CHECK(approx(moved.value->bbox.min.x, 3.0));
    CHECK(approx(moved.value->bbox.max.z, 12.0));

    auto scaled = caide::geometry::scale(box.value, 0.0, 0.0, 0.0, 2.0);
    CHECK(scaled);
    CHECK(approx(scaled.value->volume, box.value->volume * 8.0));
    CHECK(approx(scaled.value->surface_area, box.value->surface_area * 4.0));

    auto rotated = caide::geometry::rotate(box.value, 0.0, 0.0, 1.0, 90.0);
    CHECK(rotated);
    CHECK(rotated.value->surface_area == box.value->surface_area);

    auto mirrored = caide::geometry::mirror(box.value, 1.0, 0.0, 0.0, 0.0);
    CHECK(mirrored);
    CHECK(mirrored.value->volume == box.value->volume);

    auto invalid_rotate = caide::geometry::rotate(box.value, 0.0, 0.0, 0.0, 45.0);
    CHECK(!invalid_rotate);
    CHECK(caide::last_error_code() == CAIDE_ERR_INVALID_PARAM);

    auto invalid_scale = caide::geometry::scale(box.value, 0.0, 0.0, 0.0, -1.0);
    CHECK(!invalid_scale);
    CHECK(caide::last_error_code() == CAIDE_ERR_INVALID_PARAM);
    return 0;
}
