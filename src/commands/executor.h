#pragma once

#include "../internal.h"

namespace caide::commands {

Status validate_command(const CaideCommand& command);
Result<std::shared_ptr<ShapeData>> execute_command(DocumentData& doc, const CaideCommand& command);
Status execute_batch(DocumentData& doc, const std::vector<CaideCommand>& commands);

}  // namespace caide::commands
