#include <iostream>

#include "../src/error.h"
#include "../src/geometry/primitives.h"
#include "../src/references/naming.h"

#define CHECK(expr) do { if (!(expr)) { std::cerr << "CHECK failed: " #expr << " line=" << __LINE__ << '\n'; return 1; } } while (0)

int main() {
    caide::DocumentData doc;
    auto box = caide::geometry::create_box(doc, 4.0, 5.0, 6.0);
    CHECK(box);

    auto reference = caide::references::create_reference(box.value, "Body");
    CHECK(reference);
    CHECK(caide::references::is_reference_valid(reference.value) == 1);
    CHECK(!reference.value->id.empty());

    doc.shapes_by_ref[reference.value->id] = box.value;
    auto resolved = caide::references::resolve(doc, reference.value->id);
    CHECK(resolved);
    CHECK(resolved.value->reference_id == box.value->reference_id);

    auto missing = caide::references::resolve(doc, "does-not-exist");
    CHECK(!missing);
    CHECK(caide::last_error_code() == CAIDE_ERR_REFERENCE_NOT_FOUND);

    reference.value->valid = false;
    CHECK(caide::references::is_reference_valid(reference.value) == 0);
    return 0;
}
