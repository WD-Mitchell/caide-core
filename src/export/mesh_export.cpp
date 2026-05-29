#include "mesh_export.h"

#include <atomic>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>

#ifndef _WIN32
#include <unistd.h>
#else
#include <process.h>
#define getpid _getpid
#endif

#ifndef CAIDE_STUB_MODE
#include <BRepMesh_IncrementalMesh.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <Interface_Static.hxx>
#include <Message_ProgressRange.hxx>
#include <RWGltf_CafWriter.hxx>
#include <STEPControl_StepModelType.hxx>
#include <STEPControl_Writer.hxx>
#include <TCollection_AsciiString.hxx>
#include <TColStd_IndexedDataMapOfStringString.hxx>
#include <TDF_Label.hxx>
#include <TDocStd_Application.hxx>
#include <TDocStd_Document.hxx>
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#endif

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

#ifndef CAIDE_STUB_MODE

std::filesystem::path unique_temp_path(const char* suffix) {
    static std::atomic<uint64_t> counter {0};
    const auto seq = counter.fetch_add(1U, std::memory_order_relaxed);
    std::ostringstream name;
    name << "caide_export_" << ::getpid() << '_' << seq << suffix;
    return std::filesystem::temp_directory_path() / name.str();
}

std::vector<uint8_t> read_file_bytes(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        return {};
    }
    input.seekg(0, std::ios::end);
    const auto end_pos = input.tellg();
    if (end_pos <= 0) {
        return {};
    }
    const std::size_t size = static_cast<std::size_t>(end_pos);
    input.seekg(0, std::ios::beg);
    std::vector<uint8_t> bytes(size);
    input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(size));
    return bytes;
}

Status occt_write_step(const ShapeData& shape, const std::string& path) {
    if (shape.native_shape.IsNull()) {
        return set_error(CAIDE_ERR_EXPORT_FAILED, "shape has no native OCCT representation");
    }
    Interface_Static::SetCVal("write.step.schema", "AP214CD");
    Interface_Static::SetCVal("write.step.unit", "MM");
    STEPControl_Writer writer;
    const IFSelect_ReturnStatus transfer = writer.Transfer(shape.native_shape, STEPControl_AsIs);
    if (transfer != IFSelect_RetDone) {
        return set_error(CAIDE_ERR_EXPORT_FAILED, "STEPControl_Writer::Transfer failed");
    }
    const IFSelect_ReturnStatus write = writer.Write(path.c_str());
    if (write != IFSelect_RetDone) {
        return set_error(CAIDE_ERR_EXPORT_FAILED, "STEPControl_Writer::Write failed");
    }
    return Status::ok();
}

Status occt_write_gltf(const ShapeData& shape, const std::string& path, bool is_binary,
                       const CaideTessellationParams& tess) {
    if (shape.native_shape.IsNull()) {
        return set_error(CAIDE_ERR_EXPORT_FAILED, "shape has no native OCCT representation");
    }
    BRepMesh_IncrementalMesh mesher(shape.native_shape,
                                    tess.linear_deflection,
                                    tess.relative != 0,
                                    tess.angular_deflection,
                                    Standard_True);
    mesher.Perform();
    if (!mesher.IsDone()) {
        return set_error(CAIDE_ERR_EXPORT_FAILED, "BRepMesh_IncrementalMesh failed");
    }

    Handle(TDocStd_Application) app = XCAFApp_Application::GetApplication();
    Handle(TDocStd_Document) doc;
    app->NewDocument("MDTV-XCAF", doc);
    Handle(XCAFDoc_ShapeTool) shape_tool = XCAFDoc_DocumentTool::ShapeTool(doc->Main());
    shape_tool->AddShape(shape.native_shape);

    const TCollection_AsciiString out_path(path.c_str());
    RWGltf_CafWriter writer(out_path, is_binary);
    TColStd_IndexedDataMapOfStringString metadata;
    metadata.Add(TCollection_AsciiString("Generator"), TCollection_AsciiString("caide-core"));
    const bool ok = writer.Perform(doc, metadata, Message_ProgressRange());
    if (!ok) {
        return set_error(CAIDE_ERR_EXPORT_FAILED, "RWGltf_CafWriter::Perform failed");
    }
    return Status::ok();
}

std::vector<uint8_t> make_step_occt(const ShapeData& shape, Status& out_status) {
    const auto path = unique_temp_path(".step");
    auto status = occt_write_step(shape, path.string());
    if (!status) {
        out_status = status;
        std::error_code ec;
        std::filesystem::remove(path, ec);
        return {};
    }
    auto bytes = read_file_bytes(path);
    std::error_code ec;
    std::filesystem::remove(path, ec);
    if (bytes.empty()) {
        out_status = set_error(CAIDE_ERR_EXPORT_FAILED, "STEP output file was empty");
        return {};
    }
    out_status = Status::ok();
    return bytes;
}

std::vector<uint8_t> make_gltf_occt(const ShapeData& shape,
                                    const CaideTessellationParams& tess,
                                    Status& out_status) {
    const auto path = unique_temp_path(".glb");
    auto status = occt_write_gltf(shape, path.string(), /*is_binary=*/true, tess);
    if (!status) {
        out_status = status;
        std::error_code ec;
        std::filesystem::remove(path, ec);
        return {};
    }
    auto bytes = read_file_bytes(path);
    std::error_code ec;
    std::filesystem::remove(path, ec);
    if (bytes.empty()) {
        out_status = set_error(CAIDE_ERR_EXPORT_FAILED, "glTF output file was empty");
        return {};
    }
    out_status = Status::ok();
    return bytes;
}

#endif  // !CAIDE_STUB_MODE

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
        case CAIDE_FORMAT_STEP: {
#ifndef CAIDE_STUB_MODE
            if (!shape->native_shape.IsNull()) {
                Status step_status;
                buffer = make_step_occt(*shape, step_status);
                if (!step_status) {
                    return {step_status, {}};
                }
                break;
            }
#endif
            buffer = make_step_placeholder(*shape);
            break;
        }
        case CAIDE_FORMAT_GLTF: {
#ifndef CAIDE_STUB_MODE
            if (!shape->native_shape.IsNull()) {
                const CaideTessellationParams tess = normalized_params(params);
                Status gltf_status;
                buffer = make_gltf_occt(*shape, tess, gltf_status);
                if (!gltf_status) {
                    return {gltf_status, {}};
                }
                break;
            }
#endif
            buffer = make_gltf_placeholder(*shape, *mesh_result.value);
            break;
        }
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
