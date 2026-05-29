#include <cstdio>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "../src/export/mesh_export.h"
#include "../src/geometry/primitives.h"

#define CHECK(expr)                                                                                \
    do {                                                                                           \
        if (!(expr)) {                                                                             \
            std::cerr << "CHECK failed: " #expr << " line=" << __LINE__ << '\n';                   \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (0)

namespace {

std::vector<uint8_t> read_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    in.seekg(0, std::ios::end);
    const auto end = in.tellg();
    in.seekg(0, std::ios::beg);
    std::vector<uint8_t> bytes(static_cast<std::size_t>(end));
    in.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return bytes;
}

}  // namespace

int main() {
    caide::DocumentData doc;
    auto box = caide::geometry::create_box(doc, 1.0, 1.0, 1.0);
    CHECK(box);

    const std::string out_path = "test_export_gltf_cube.glb";
    auto status = caide::exporter::export_to_file(box.value, CAIDE_FORMAT_GLTF, out_path, nullptr);
    CHECK(static_cast<bool>(status));

    auto bytes = read_file(out_path);
    std::cout << "glTF cube file size: " << bytes.size() << " bytes" << std::endl;
    CHECK(bytes.size() > 32U);

#ifdef CAIDE_STUB_MODE
    CHECK(bytes[0] == '{');
#else
    CHECK(bytes[0] == 0x67 && bytes[1] == 0x6C && bytes[2] == 0x54 && bytes[3] == 0x46);

    uint32_t version = 0;
    uint32_t total_len = 0;
    std::memcpy(&version, bytes.data() + 4, sizeof(uint32_t));
    std::memcpy(&total_len, bytes.data() + 8, sizeof(uint32_t));
    CHECK(version == 2U);
    CHECK(total_len == bytes.size());

    uint32_t json_len = 0;
    uint32_t json_kind = 0;
    std::memcpy(&json_len, bytes.data() + 12, sizeof(uint32_t));
    std::memcpy(&json_kind, bytes.data() + 16, sizeof(uint32_t));
    CHECK(json_kind == 0x4E4F534A);  // 'JSON'
    CHECK(json_len > 0U);
    CHECK(20U + json_len <= bytes.size());

    const std::string json(reinterpret_cast<const char*>(bytes.data()) + 20, json_len);
    CHECK(json.find("\"asset\"") != std::string::npos);
    CHECK(json.find("\"version\"") != std::string::npos);
    CHECK(json.find("\"2.0\"") != std::string::npos);
    CHECK(json.find("\"meshes\"") != std::string::npos);
    std::cout << "glTF JSON chunk length: " << json_len << " bytes" << std::endl;
#endif

    std::remove(out_path.c_str());
    std::cout << "test_export_gltf passed" << std::endl;
    return 0;
}
