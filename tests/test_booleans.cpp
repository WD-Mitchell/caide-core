#include <iostream>

#include "../src/error.h"
#include "../src/geometry/booleans.h"
#include "../src/geometry/primitives.h"

#define CHECK(expr) do { if (!(expr)) { std::cerr << "CHECK failed: " #expr << " line=" << __LINE__ << '\n'; return 1; } } while (0)

int main() {
    caide::DocumentData doc;
    auto a = caide::geometry::create_box(doc, 20.0, 20.0, 20.0);
    auto b = caide::geometry::create_box(doc, 10.0, 10.0, 10.0);
    CHECK(a && b);

    auto uni = caide::geometry::boolean_union(a.value, b.value);
    CHECK(uni);
    CHECK(uni.value->volume >= a.value->volume);
    CHECK(uni.value->bbox.max.x == a.value->bbox.max.x);

    auto sub = caide::geometry::boolean_subtract(a.value, b.value);
    CHECK(sub);
    CHECK(sub.value->volume <= a.value->volume);
    CHECK(sub.value->children.size() == 2U);

    auto inter = caide::geometry::boolean_intersect(a.value, b.value);
    CHECK(inter);
    CHECK(inter.value->volume <= b.value->volume);
    CHECK(inter.value->bbox.max.x <= a.value->bbox.max.x);

    auto clone = caide::clone_shape_data(inter.value);
    CHECK(clone->children.size() == 2U);
    CHECK(clone->kind == caide::ShapeKind::BooleanIntersect);

    auto far_box = caide::make_shape(caide::ShapeKind::Box, "far", caide::make_bbox({100.0, 100.0, 100.0}, {110.0, 110.0, 110.0}), 1000.0, 600.0);
    auto bad = caide::geometry::boolean_intersect(a.value, far_box);
    CHECK(!bad);
    CHECK(caide::last_error_code() == CAIDE_ERR_TOPOLOGY_ERROR);
    CHECK(caide::last_error_message()[0] != '\0');
    return 0;
}
