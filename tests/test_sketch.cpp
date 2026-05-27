#include <cmath>
#include <iostream>

#include "../src/geometry/sketch.h"

#define CHECK(expr) do { if (!(expr)) { std::cerr << "CHECK failed: " #expr << " line=" << __LINE__ << '\n'; return 1; } } while (0)

int main() {
    auto doc = std::make_shared<caide::DocumentData>();
    auto sketch = caide::geometry::create_sketch(doc);
    CHECK(sketch);

    CHECK(caide::geometry::add_line(*sketch.value, 0.0, 0.0, 10.0, 0.0));
    CHECK(caide::geometry::add_line(*sketch.value, 10.0, 0.0, 10.0, 10.0));
    CHECK(caide::geometry::add_line(*sketch.value, 10.0, 10.0, 0.0, 10.0));
    CHECK(caide::geometry::add_line(*sketch.value, 0.0, 10.0, 0.0, 0.0));
    CHECK(caide::geometry::close_sketch(*sketch.value));

    auto wire = caide::geometry::sketch_to_wire(*sketch.value);
    CHECK(wire);
    CHECK(wire.value->closed);
    CHECK(wire.value->sampled_points.size() >= 4U);

    auto face = caide::geometry::sketch_to_face(*sketch.value);
    CHECK(face);
    CHECK(std::fabs(face.value->surface_area - 100.0) < 1e-6);

    auto circle_sketch = caide::geometry::create_sketch(doc);
    CHECK(circle_sketch);
    CHECK(caide::geometry::add_circle(*circle_sketch.value, 0.0, 0.0, 5.0));
    auto circle_face = caide::geometry::sketch_to_face(*circle_sketch.value);
    CHECK(circle_face);
    CHECK(circle_face.value->surface_area > 75.0);

    auto spline_sketch = caide::geometry::create_sketch(doc);
    CHECK(spline_sketch);
    const double spline_points[] = {0.0, 0.0, 2.0, 3.0, 4.0, 3.0, 6.0, 0.0};
    CHECK(caide::geometry::add_spline(*spline_sketch.value, spline_points, 8));
    CHECK(caide::geometry::add_arc(*spline_sketch.value, 3.0, 0.0, 3.0, 0.0, 180.0));
    auto spline_wire = caide::geometry::sketch_to_wire(*spline_sketch.value);
    CHECK(spline_wire);
    CHECK(!spline_wire.value->sampled_points.empty());
    return 0;
}
