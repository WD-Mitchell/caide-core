#include <cstdio>
#include <iostream>
#include <string>

#include "../src/export/mesh_export.h"
#include "../src/geometry/primitives.h"

#ifndef CAIDE_STUB_MODE
#include <BRepTools.hxx>
#include <STEPControl_Reader.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Shape.hxx>
#include <TopAbs_ShapeEnum.hxx>
#endif

#define CHECK(expr)                                                                                \
    do {                                                                                           \
        if (!(expr)) {                                                                             \
            std::cerr << "CHECK failed: " #expr << " line=" << __LINE__ << '\n';                   \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (0)

namespace {

#ifndef CAIDE_STUB_MODE
std::size_t count_shapes(const TopoDS_Shape& shape, TopAbs_ShapeEnum kind) {
    std::size_t count = 0;
    for (TopExp_Explorer exp(shape, kind); exp.More(); exp.Next()) {
        ++count;
    }
    return count;
}
#endif

}  // namespace

int main() {
    caide::DocumentData doc;
    auto box = caide::geometry::create_box(doc, 1.0, 1.0, 1.0);
    CHECK(box);

    const std::string out_path = "test_export_step_cube.step";
    auto status = caide::exporter::export_to_file(box.value, CAIDE_FORMAT_STEP, out_path, nullptr);
    CHECK(static_cast<bool>(status));

    std::FILE* fp = std::fopen(out_path.c_str(), "rb");
    CHECK(fp != nullptr);
    std::fseek(fp, 0, SEEK_END);
    const long size = std::ftell(fp);
    std::fclose(fp);
    CHECK(size > 256);
    std::cout << "STEP cube file size: " << size << " bytes" << std::endl;

#ifndef CAIDE_STUB_MODE
    STEPControl_Reader reader;
    const auto read_status = reader.ReadFile(out_path.c_str());
    CHECK(read_status == IFSelect_RetDone);
    const Standard_Integer roots = reader.TransferRoots();
    CHECK(roots > 0);
    TopoDS_Shape read_shape = reader.OneShape();
    CHECK(!read_shape.IsNull());

    const std::size_t in_faces = count_shapes(box.value->native_shape, TopAbs_FACE);
    const std::size_t in_edges = count_shapes(box.value->native_shape, TopAbs_EDGE);
    const std::size_t in_vertices = count_shapes(box.value->native_shape, TopAbs_VERTEX);
    const std::size_t out_faces = count_shapes(read_shape, TopAbs_FACE);
    const std::size_t out_edges = count_shapes(read_shape, TopAbs_EDGE);
    const std::size_t out_vertices = count_shapes(read_shape, TopAbs_VERTEX);
    std::cout << "STEP topology: in faces=" << in_faces << " edges=" << in_edges
              << " vertices=" << in_vertices << "; out faces=" << out_faces
              << " edges=" << out_edges << " vertices=" << out_vertices << std::endl;
    CHECK(in_faces == out_faces);
    CHECK(in_edges == out_edges);
    CHECK(in_vertices == out_vertices);
#endif

    std::remove(out_path.c_str());
    std::cout << "test_export_step passed" << std::endl;
    return 0;
}
