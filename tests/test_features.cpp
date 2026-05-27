#include <iostream>

#include "../src/error.h"
#include "../src/geometry/features.h"
#include "../src/geometry/primitives.h"

#define CHECK(expr) do { if (!(expr)) { std::cerr << "CHECK failed: " #expr << " line=" << __LINE__ << '\n'; return 1; } } while (0)

int main() {
    caide::DocumentData doc;
    auto box = caide::geometry::create_box(doc, 20.0, 20.0, 20.0);
    CHECK(box);

    std::vector<int> edges {0, 1, 2};
    auto fillet = caide::geometry::fillet(box.value, 1.0, edges);
    CHECK(fillet);
    CHECK(fillet.value->volume < box.value->volume);
    CHECK(fillet.value->history_tags.back() == "fillet");

    auto chamfer = caide::geometry::chamfer(box.value, 1.5, edges);
    CHECK(chamfer);
    CHECK(chamfer.value->surface_area <= box.value->surface_area);
    CHECK(chamfer.value->params.size() == 4U);

    std::vector<int> faces {0};
    auto shell = caide::geometry::shell(box.value, 1.0, faces);
    CHECK(shell);
    CHECK(shell.value->volume < box.value->volume);
    CHECK(shell.value->metadata.at("feature") == "shell");

    auto draft = caide::geometry::draft(box.value, 3.0, {0.0, 0.0, 1.0}, faces);
    CHECK(draft);
    CHECK(draft.value->bbox.max.x >= box.value->bbox.max.x);
    CHECK(draft.value->params.size() >= 4U);

    auto bad_shell = caide::geometry::shell(box.value, 50.0, faces);
    CHECK(!bad_shell);
    CHECK(caide::last_error_code() == CAIDE_ERR_INVALID_PARAM);

    auto bad_fillet = caide::geometry::fillet(box.value, -1.0, edges);
    CHECK(!bad_fillet);
    CHECK(caide::last_error_code() == CAIDE_ERR_INVALID_PARAM);
    return 0;
}
