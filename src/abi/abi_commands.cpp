#include "../commands/executor.h"
#include "../internal.h"

extern "C" {

CaideError caide_command_execute(CaideDocument doc, const CaideCommand* cmd, CaideShape* out_result) {
    if (doc == nullptr || doc->impl == nullptr) {
        return caide::set_error(CAIDE_ERR_NULL_HANDLE, "document is null").code;
    }
    if (cmd == nullptr || out_result == nullptr) {
        return caide::set_error(CAIDE_ERR_INVALID_PARAM, "cmd and out_result are required").code;
    }
    auto result = caide::commands::execute_command(*doc->impl, *cmd);
    if (!result) {
        return result.status.code;
    }
    *out_result = caide::make_shape_handle(result.value);
    return CAIDE_OK;
}

CaideError caide_command_execute_batch(CaideDocument doc, const CaideCommand* cmds, int cmd_count) {
    if (doc == nullptr || doc->impl == nullptr) {
        return caide::set_error(CAIDE_ERR_NULL_HANDLE, "document is null").code;
    }
    if (cmds == nullptr || cmd_count < 0) {
        return caide::set_error(CAIDE_ERR_INVALID_PARAM, "cmds and cmd_count must be valid").code;
    }
    std::vector<CaideCommand> batch;
    for (int i = 0; i < cmd_count; ++i) {
        batch.push_back(cmds[i]);
    }
    return caide::commands::execute_batch(*doc->impl, batch).code;
}

CaideError caide_command_validate(const CaideCommand* cmd) {
    if (cmd == nullptr) {
        return caide::set_error(CAIDE_ERR_INVALID_PARAM, "cmd is null").code;
    }
    return caide::commands::validate_command(*cmd).code;
}

}  // extern "C"
