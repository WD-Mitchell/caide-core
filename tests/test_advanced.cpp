#include <iostream>

#include "../src/error.h"
#include "../src/geometry/advanced.h"
#include "../src/geometry/sketch.h"

#define CHECK(expr) do { if (!(expr)) { std::cerr << "CHECK failed: " #expr << " line=" << __LINE__ << '\n'; return 1; } } while (0)

int main() {
    auto doc = std::make_shared<caide::DocumentData>();
    auto sketch = caide::geometry::create_sketch(doc);
    CHECK(sketch);
    CHECK(caide::geometry::add_line(*sketch.value, 0.0, 0.0, 5.0, 0.0));
    CHECK(caide::geometry::add_line(*sketch.value, 5.0, 0.0, 5.0, 5.0));
    CHECK(caide::geometry::add_line(*sketch.value, 5.0, 5.0, 0.0, 5.0));
    CHECK(caide::geometry::add_line(*sketch.value, 0.0, 5.0, 0.0, 0.0));
    CHECK(caide::geometry::close_sketch(*sketch.value));

    auto face = caide::geometry::sketch_to_face(*sketch.value);
    CHECK(face);
    auto wire = caide::geometry::sketch_to_wire(*sketch.value);
    CHECK(wire);

    auto extruded = caide::geometry::extrude(face.value, 0.0, 0.0, 20.0);
    CHECK(extruded);
    CHECK(extruded.value->volume > 0.0);
    CHECK(extruded.value->children.size() == 1U);

    auto revolved = caide::geometry::revolve(face.value, 0.0, 1.0, 0.0, 180.0);
    CHECK(revolved);
    CHECK(revolved.value->volume > 0.0);

    auto swept = caide::geometry::sweep(face.value, wire.value);
    CHECK(swept);
    CHECK(swept.value->volume > 0.0);
    CHECK(swept.value->children.size() == 2U);

    std::vector<std::shared_ptr<caide::ShapeData>> profiles {wire.value, wire.value};
    auto lofted = caide::geometry::loft(profiles);
    CHECK(lofted);
    CHECK(lofted.value->volume > 0.0);

    std::vector<std::shared_ptr<caide::ShapeData>> invalid_profiles {wire.value};
    auto invalid_loft = caide::geometry::loft(invalid_profiles);
    CHECK(!invalid_loft);
    CHECK(caide::last_error_code() == CAIDE_ERR_INVALID_PARAM);
    return 0;
}
