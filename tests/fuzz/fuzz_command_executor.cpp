// libFuzzer harness for the caide_core command executor.
//
// Treats the input bytes as a sequence of length-prefixed (type, params, ref)
// strings, builds CaideCommand structs and feeds them through the public C
// ABI executor. All expected error codes are swallowed; we only crash on UB,
// asserts, sanitizer findings, or unexpected post-conditions.

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

#include "caide_core/caide_core.h"

namespace {

// Read up to `cap` bytes from the cursor as a NUL-stripped UTF-8-ish string.
std::string take_string(const uint8_t*& cur, const uint8_t* end, size_t cap) {
    if (cur >= end) {
        return {};
    }
    const size_t available = static_cast<size_t>(end - cur);
    uint8_t len_byte = *cur++;
    size_t want = static_cast<size_t>(len_byte) % (cap + 1);
    if (want > available - 1) {
        want = available - 1;
    }
    std::string s(reinterpret_cast<const char*>(cur), want);
    cur += want;
    // Strip embedded NULs so the bytes round-trip safely through c_char_p.
    for (char& c : s) {
        if (c == '\0') c = ' ';
    }
    return s;
}

bool is_expected_error(CaideError code) {
    switch (code) {
        case CAIDE_OK:
        case CAIDE_ERR_NULL_HANDLE:
        case CAIDE_ERR_INVALID_PARAM:
        case CAIDE_ERR_OPERATION_FAILED:
        case CAIDE_ERR_TOPOLOGY_ERROR:
        case CAIDE_ERR_REFERENCE_NOT_FOUND:
        case CAIDE_ERR_EXPORT_FAILED:
        case CAIDE_ERR_COMMAND_INVALID:
        case CAIDE_ERR_HISTORY_EMPTY:
        case CAIDE_ERR_OUT_OF_MEMORY:
        case CAIDE_ERR_NOT_IMPLEMENTED:
        case CAIDE_ERR_THREAD_SAFETY:
            return true;
    }
    return false;
}

}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size == 0) {
        return 0;
    }

    CaideContext ctx = nullptr;
    if (caide_context_create(&ctx) != CAIDE_OK) {
        return 0;
    }
    CaideDocument doc = nullptr;
    if (caide_document_create(ctx, &doc) != CAIDE_OK) {
        caide_context_destroy(ctx);
        return 0;
    }

    const uint8_t* cur = data;
    const uint8_t* end = data + size;

    // Cap commands per input so we don't time out on huge corpora.
    constexpr int kMaxCommands = 32;
    int executed = 0;
    std::vector<std::string> last_refs;

    while (cur < end && executed < kMaxCommands) {
        std::string type = take_string(cur, end, 32);
        std::string params = take_string(cur, end, 128);
        std::string target_ref = take_string(cur, end, 32);

        // Bias half the inputs toward referencing a previously-produced shape
        // so the executor exercises its reference-resolution paths.
        if (!last_refs.empty() && (params.size() & 1u)) {
            target_ref = last_refs[params.size() % last_refs.size()];
        }

        CaideCommand cmd;
        cmd.type = type.c_str();
        cmd.params_json = params.c_str();
        cmd.target_ref = target_ref.empty() ? nullptr : target_ref.c_str();

        // Validation should never crash; ignore result.
        (void)caide_command_validate(&cmd);

        CaideShape result = nullptr;
        CaideError code = caide_command_execute(doc, &cmd, &result);
        if (!is_expected_error(code)) {
            __builtin_trap();
        }
        if (code == CAIDE_OK && result != nullptr) {
            // Probe a few read-only queries to make sure the result handle
            // is usable without crashing.
            double volume = 0.0;
            (void)caide_shape_volume(result, &volume);
            int valid = caide_shape_is_valid(result);
            (void)valid;
            caide_shape_destroy(result);
        }
        ++executed;
    }

    // Drive a small undo/redo burst — must never crash regardless of state.
    for (int i = 0; i < 4 && i < executed; ++i) {
        (void)caide_document_undo(doc);
    }
    for (int i = 0; i < 4; ++i) {
        (void)caide_document_redo(doc);
    }

    caide_document_destroy(doc);
    caide_context_destroy(ctx);
    return 0;
}

#include "standalone_main.inc"
