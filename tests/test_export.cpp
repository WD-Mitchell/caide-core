#include <iostream>
#include <string>

#include "../src/export/mesh_export.h"
#include "../src/geometry/primitives.h"

#define CHECK(expr) do { if (!(expr)) { std::cerr << "CHECK failed: " #expr << " line=" << __LINE__ << '\n'; return 1; } } while (0)

int main() {
    caide::DocumentData doc;
    auto box = caide::geometry::create_box(doc, 10.0, 10.0, 10.0);
    CHECK(box);

    auto mesh = caide::exporter::tessellate(box.value, nullptr);
    CHECK(mesh);
    CHECK(!mesh.value->vertices.empty());
    CHECK(!mesh.value->indices.empty());
    CHECK(mesh.value->vertices.size() == mesh.value->normals.size());

    auto obj = caide::exporter::export_to_buffer(box.value, CAIDE_FORMAT_OBJ, nullptr);
    CHECK(obj);
    CHECK(obj.value.size() > 16U);
    CHECK(std::string(obj.value.begin(), obj.value.begin() + 1) == "#");

    auto stl = caide::exporter::export_to_buffer(box.value, CAIDE_FORMAT_STL_BINARY, nullptr);
    CHECK(stl);
    CHECK(stl.value.size() > 84U);

    auto mf3 = caide::exporter::export_to_buffer(box.value, CAIDE_FORMAT_3MF, nullptr);
    CHECK(mf3);
    CHECK(std::string(mf3.value.begin(), mf3.value.begin() + 5) == "<?xml");

    auto step = caide::exporter::export_to_buffer(box.value, CAIDE_FORMAT_STEP, nullptr);
    CHECK(step);
    CHECK(step.value.size() > 32U);

    auto gltf = caide::exporter::export_to_buffer(box.value, CAIDE_FORMAT_GLTF, nullptr);
    CHECK(gltf);
    CHECK(std::string(gltf.value.begin(), gltf.value.begin() + 1) == "{");
    return 0;
}
