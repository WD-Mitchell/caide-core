#pragma once

#include "../internal.h"

namespace caide::commands {

Status undo(DocumentData& doc);
Status redo(DocumentData& doc);
int history_count(DocumentData& doc);
Result<std::shared_ptr<ShapeData>> find_shape(DocumentData& doc, const std::string& ref_id);

}  // namespace caide::commands
