#pragma once

#include "../internal.h"

namespace caide::references {

Result<std::shared_ptr<ReferenceData>> create_reference(const std::shared_ptr<ShapeData>& shape, const std::string& label);
Result<std::shared_ptr<ShapeData>> resolve(DocumentData& doc, const std::string& ref_id);
int is_reference_valid(const std::shared_ptr<ReferenceData>& reference);

}  // namespace caide::references
