#include "mesh_export.h"

#include <cstring>
#include <fstream>
#include <sstream>

namespace caide::exporter {

CaideTessellationParams normalized_params(const CaideTessellationParams* params) {
    CaideTessellationParams out {};
    out.linear_deflection = params != nullptr && params->linear_deflection > 0.0 ? params->linear_deflection : 0.1;
    out.angular_deflection = params != nullptr && params->angular_deflection > 0.0 ? params->angular_deflection : 0.5;
    out.relative = params != nullptr ? params->relative : 0;
    return out;
}

namespace {

std::vector<uint8_t> make_ascii_stl(const MeshData& mesh) {
    std::ostringstream stream;
    stream << "solid caide\n";
    for (std::size_t i = 0; i < mesh.indices.size(); i += 3) {
        const uint32_t ia = mesh.indices[i] * 3U;
        const uint32_t ib = mesh.indices[i + 1] * 3U;
        const uint32_t ic = mesh.indices[i + 2] * 3U;
        stream << "  facet normal 0 0 1\n";
        stream << "    outer loop\n";
        stream << "      vertex " << mesh.vertices[ia] << ' ' << mesh.vertices[ia + 1] << ' ' << mesh.vertices[ia + 2] << "\n";
        stream << "      vertex " << mesh.vertices[ib] << ' ' << mesh.vertices[ib + 1] << ' ' << mesh.vertices[ib + 2] << "\n";
        stream << "      vertex " << mesh.vertices[ic] << ' ' << mesh.vertices[ic + 1] << ' ' << mesh.vertices[ic + 2] << "\n";
        stream << "    endloop\n";
        stream << "  endfacet\n";
    }
    stream << "endsolid caide\n";
    const std::string text = stream.str();
    return std::vector<uint8_t>(text.begin(), text.end());
}

std::vector<uint8_t> make_binary_stl(const MeshData& mesh) {
    const uint32_t triangle_count = static_cast<uint32_t>(mesh.indices.size() / 3U);
    std::vector<uint8_t> bytes(84U + (triangle_count * 50U), 0U);
    const char* header = "caide-core stub binary stl";
    std::memcpy(bytes.data(), header, std::strlen(header));
    std::memcpy(bytes.data() + 80U, &triangle_count, sizeof(uint32_t));
    std::size_t offset = 84U;
    for (std::size_t i = 0; i < mesh.indices.size(); i += 3) {
        const uint32_t ia = mesh.indices[i] * 3U;
        const uint32_t ib = mesh.indices[i + 1] * 3U;
        const uint32_t ic = mesh.indices[i + 2] * 3U;
        const float normal[3] = {0.0F, 0.0F, 1.0F};
        const float vertices[9] = {
            mesh.vertices[ia], mesh.vertices[ia + 1], mesh.vertices[ia + 2],
            mesh.vertices[ib], mesh.vertices[ib + 1], mesh.vertices[ib + 2],
            mesh.vertices[ic], mesh.vertices[ic + 1], mesh.vertices[ic + 2],
        };
        std::memcpy(bytes.data() + offset, normal, sizeof(normal));
        offset += sizeof(normal);
        std::memcpy(bytes.data() + offset, vertices, sizeof(vertices));
        offset += sizeof(vertices);
        const uint16_t attribute = 0;
        std::memcpy(bytes.data() + offset, &attribute, sizeof(attribute));
        offset += sizeof(attribute);
    }
    return bytes;
}

std::vector<uint8_t> make_obj(const MeshData& mesh) {
    std::ostringstream stream;
    stream << "# caide-core generated OBJ\n";
    for (std::size_t i = 0; i < mesh.vertices.size(); i += 3) {
        stream << "v " << mesh.vertices[i] << ' ' << mesh.vertices[i + 1] << ' ' << mesh.vertices[i + 2] << "\n";
    }
    for (std::size_t i = 0; i < mesh.normals.size(); i += 3) {
        stream << "vn " << mesh.normals[i] << ' ' << mesh.normals[i + 1] << ' ' << mesh.normals[i + 2] << "\n";
    }
    for (std::size_t i = 0; i < mesh.indices.size(); i += 3) {
        stream << "f "
               << mesh.indices[i] + 1U << "//" << mesh.indices[i] + 1U << ' '
               << mesh.indices[i + 1] + 1U << "//" << mesh.indices[i + 1] + 1U << ' '
               << mesh.indices[i + 2] + 1U << "//" << mesh.indices[i + 2] + 1U << "\n";
    }
    const std::string text = stream.str();
    return std::vector<uint8_t>(text.begin(), text.end());
}

std::vector<uint8_t> make_3mf(const ShapeData& shape, const MeshData& mesh) {
    std::ostringstream stream;
    stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    stream << "<model unit=\"millimeter\" xml:lang=\"en-US\" xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\">\n";
    stream << "  <resources>\n";
    stream << "    <object id=\"1\" type=\"model\">\n";
    stream << "      <mesh>\n";
    stream << "        <vertices>\n";
    for (std::size_t i = 0; i < mesh.vertices.size(); i += 3) {
        stream << "          <vertex x=\"" << mesh.vertices[i] << "\" y=\"" << mesh.vertices[i + 1] << "\" z=\"" << mesh.vertices[i + 2] << "\"/>\n";
    }
    stream << "        </vertices>\n";
    stream << "        <triangles>\n";
    for (std::size_t i = 0; i < mesh.indices.size(); i += 3) {
        stream << "          <triangle v1=\"" << mesh.indices[i] << "\" v2=\"" << mesh.indices[i + 1] << "\" v3=\"" << mesh.indices[i + 2] << "\"/>\n";
    }
    stream << "        </triangles>\n";
    stream << "      </mesh>\n";
    stream << "    </object>\n";
    stream << "  </resources>\n";
    stream << "  <build><item objectid=\"1\"/></build>\n";
    stream << "  <!-- bbox=" << format_double(shape.bbox.min.x) << ',' << format_double(shape.bbox.min.y) << ',' << format_double(shape.bbox.min.z)
           << " -> " << format_double(shape.bbox.max.x) << ',' << format_double(shape.bbox.max.y) << ',' << format_double(shape.bbox.max.z) << " -->\n";
    stream << "</model>\n";
    const std::string text = stream.str();
    return std::vector<uint8_t>(text.begin(), text.end());
}

std::vector<uint8_t> make_step_placeholder(const ShapeData& shape) {
    std::ostringstream stream;
    stream << "ISO-10303-21;\nHEADER;\nFILE_DESCRIPTION(('caide-core stub export'),'2;1');\nENDSEC;\nDATA;\n";
    stream << "/* shape kind: " << shape_kind_name(shape.kind) << " */\n";
    stream << "/* bbox: " << format_double(shape.bbox.min.x) << ',' << format_double(shape.bbox.min.y) << ',' << format_double(shape.bbox.min.z)
           << " -> " << format_double(shape.bbox.max.x) << ',' << format_double(shape.bbox.max.y) << ',' << format_double(shape.bbox.max.z) << " */\n";
    stream << "ENDSEC;\nEND-ISO-10303-21;\n";
    const std::string text = stream.str();
    return std::vector<uint8_t>(text.begin(), text.end());
}

std::vector<uint8_t> make_gltf_placeholder(const ShapeData& shape, const MeshData& mesh) {
    std::ostringstream stream;
    stream << "{\n";
    stream << "  \"asset\": { \"version\": \"2.0\", \"generator\": \"caide-core\" },\n";
    stream << "  \"extras\": {\n";
    stream << "    \"shapeKind\": \"" << shape_kind_name(shape.kind) << "\",\n";
    stream << "    \"vertexCount\": " << (mesh.vertices.size() / 3U) << ",\n";
    stream << "    \"triangleCount\": " << (mesh.indices.size() / 3U) << "\n";
    stream << "  }\n";
    stream << "}\n";
    const std::string text = stream.str();
    return std::vector<uint8_t>(text.begin(), text.end());
}

}  // namespace

Result<std::vector<uint8_t>> export_to_buffer(const std::shared_ptr<ShapeData>& shape, CaideExportFormat format, const CaideTessellationParams* params) {
    auto required = require_shape(shape, "shape");
    if (!required) {
        return {required.status, {}};
    }
    auto mesh_result = tessellate(shape, params);
    if (!mesh_result) {
        return {mesh_result.status, {}};
    }

    std::vector<uint8_t> buffer;
    switch (format) {
        case CAIDE_FORMAT_STL_ASCII:
            buffer = make_ascii_stl(*mesh_result.value);
            break;
        case CAIDE_FORMAT_STL_BINARY:
            buffer = make_binary_stl(*mesh_result.value);
            break;
        case CAIDE_FORMAT_OBJ:
            buffer = make_obj(*mesh_result.value);
            break;
        case CAIDE_FORMAT_3MF:
            buffer = make_3mf(*shape, *mesh_result.value);
            break;
        case CAIDE_FORMAT_STEP:
            buffer = make_step_placeholder(*shape);
            break;
        case CAIDE_FORMAT_GLTF:
            buffer = make_gltf_placeholder(*shape, *mesh_result.value);
            break;
        default:
            return {set_error(CAIDE_ERR_EXPORT_FAILED, "unsupported export format"), {}};
    }
    clear_error();
    return {Status::ok(), buffer};
}

Status export_to_file(const std::shared_ptr<ShapeData>& shape, CaideExportFormat format, const std::string& path, const CaideTessellationParams* params) {
    auto buffer_result = export_to_buffer(shape, format, params);
    if (!buffer_result) {
        return buffer_result.status;
    }
    std::ofstream output(path, std::ios::binary);
    if (!output.is_open()) {
        return set_error(CAIDE_ERR_EXPORT_FAILED, "unable to open export path: " + path);
    }
    output.write(reinterpret_cast<const char*>(buffer_result.value.data()), static_cast<std::streamsize>(buffer_result.value.size()));
    if (!output.good()) {
        return set_error(CAIDE_ERR_EXPORT_FAILED, "failed to write export file: " + path);
    }
    clear_error();
    return Status::ok();
}

}  // namespace caide::exporter
