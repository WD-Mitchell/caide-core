#include "../export/mesh_export.h"
#include "../internal.h"

extern "C" {

CaideError caide_export_to_file(CaideShape shape, CaideExportFormat format, const char* file_path, const CaideTessellationParams* params) {
    if (shape == nullptr || shape->impl == nullptr) {
        return caide::set_error(CAIDE_ERR_NULL_HANDLE, "shape is null").code;
    }
    if (file_path == nullptr) {
        return caide::set_error(CAIDE_ERR_INVALID_PARAM, "file_path cannot be null").code;
    }
    return caide::exporter::export_to_file(shape->impl, format, file_path, params).code;
}

CaideError caide_export_to_buffer(CaideShape shape, CaideExportFormat format, const CaideTessellationParams* params, uint8_t** out_buffer, size_t* out_size) {
    if (shape == nullptr || shape->impl == nullptr) {
        return caide::set_error(CAIDE_ERR_NULL_HANDLE, "shape is null").code;
    }
    if (out_buffer == nullptr || out_size == nullptr) {
        return caide::set_error(CAIDE_ERR_INVALID_PARAM, "out_buffer and out_size are required").code;
    }
    auto result = caide::exporter::export_to_buffer(shape->impl, format, params);
    if (!result) {
        return result.status.code;
    }
    uint8_t* buffer = static_cast<uint8_t*>(std::malloc(result.value.size()));
    if (buffer == nullptr && !result.value.empty()) {
        return caide::set_error(CAIDE_ERR_OUT_OF_MEMORY, "failed to allocate export buffer").code;
    }
    if (!result.value.empty()) {
        std::memcpy(buffer, result.value.data(), result.value.size());
    }
    *out_buffer = buffer;
    *out_size = result.value.size();
    caide::clear_error();
    return CAIDE_OK;
}

void caide_buffer_free(uint8_t* buffer) {
    std::free(buffer);
    caide::clear_error();
}

CaideError caide_mesh_tessellate(CaideShape shape, const CaideTessellationParams* params, CaideMesh* out_mesh) {
    if (shape == nullptr || shape->impl == nullptr) {
        return caide::set_error(CAIDE_ERR_NULL_HANDLE, "shape is null").code;
    }
    if (out_mesh == nullptr) {
        return caide::set_error(CAIDE_ERR_INVALID_PARAM, "out_mesh cannot be null").code;
    }
    auto mesh = caide::exporter::tessellate(shape->impl, params);
    if (!mesh) {
        return mesh.status.code;
    }
    *out_mesh = caide::make_mesh_handle(mesh.value);
    return CAIDE_OK;
}

void caide_mesh_destroy(CaideMesh mesh) {
    delete mesh;
    caide::clear_error();
}

int caide_mesh_vertex_count(CaideMesh mesh) {
    if (mesh == nullptr || mesh->impl == nullptr) {
        caide::set_error(CAIDE_ERR_NULL_HANDLE, "mesh is null");
        return 0;
    }
    return static_cast<int>(mesh->impl->vertices.size() / 3U);
}

int caide_mesh_triangle_count(CaideMesh mesh) {
    if (mesh == nullptr || mesh->impl == nullptr) {
        caide::set_error(CAIDE_ERR_NULL_HANDLE, "mesh is null");
        return 0;
    }
    return static_cast<int>(mesh->impl->indices.size() / 3U);
}

CaideError caide_mesh_get_vertices(CaideMesh mesh, float** out_vertices, int* out_count) {
    if (mesh == nullptr || mesh->impl == nullptr) {
        return caide::set_error(CAIDE_ERR_NULL_HANDLE, "mesh is null").code;
    }
    if (out_vertices == nullptr || out_count == nullptr) {
        return caide::set_error(CAIDE_ERR_INVALID_PARAM, "out_vertices and out_count are required").code;
    }
    *out_vertices = mesh->impl->vertices.data();
    *out_count = static_cast<int>(mesh->impl->vertices.size());
    caide::clear_error();
    return CAIDE_OK;
}

CaideError caide_mesh_get_normals(CaideMesh mesh, float** out_normals, int* out_count) {
    if (mesh == nullptr || mesh->impl == nullptr) {
        return caide::set_error(CAIDE_ERR_NULL_HANDLE, "mesh is null").code;
    }
    if (out_normals == nullptr || out_count == nullptr) {
        return caide::set_error(CAIDE_ERR_INVALID_PARAM, "out_normals and out_count are required").code;
    }
    *out_normals = mesh->impl->normals.data();
    *out_count = static_cast<int>(mesh->impl->normals.size());
    caide::clear_error();
    return CAIDE_OK;
}

CaideError caide_mesh_get_indices(CaideMesh mesh, uint32_t** out_indices, int* out_count) {
    if (mesh == nullptr || mesh->impl == nullptr) {
        return caide::set_error(CAIDE_ERR_NULL_HANDLE, "mesh is null").code;
    }
    if (out_indices == nullptr || out_count == nullptr) {
        return caide::set_error(CAIDE_ERR_INVALID_PARAM, "out_indices and out_count are required").code;
    }
    *out_indices = mesh->impl->indices.data();
    *out_count = static_cast<int>(mesh->impl->indices.size());
    caide::clear_error();
    return CAIDE_OK;
}

}  // extern "C"
