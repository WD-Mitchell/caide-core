#pragma once

#include "../internal.h"

namespace caide::exporter {

CaideTessellationParams normalized_params(const CaideTessellationParams* params);
Result<std::shared_ptr<MeshData>> tessellate(const std::shared_ptr<ShapeData>& shape, const CaideTessellationParams* params);
Result<std::vector<uint8_t>> export_to_buffer(const std::shared_ptr<ShapeData>& shape, CaideExportFormat format, const CaideTessellationParams* params);
Status export_to_file(const std::shared_ptr<ShapeData>& shape, CaideExportFormat format, const std::string& path, const CaideTessellationParams* params);

}  // namespace caide::exporter
