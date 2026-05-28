// libFuzzer harness for the undo/redo history stack.
//
// Builds a document, applies a fuzz-driven sequence of (do, undo, redo)
// operations, and asserts the documented invariants:
//
//   * history_count is monotonically updated by execute/undo/redo
//   * undo + redo == identity (history_count returns to its previous value)
//   * history_count never goes negative
//   * undo on an empty history returns CAIDE_ERR_HISTORY_EMPTY (not a crash)
//   * redo without prior undo returns CAIDE_ERR_HISTORY_EMPTY (not a crash)

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string>

#include "caide_core/caide_core.h"

namespace {

constexpr int kMaxOps = 64;

// Static menu of deterministic, well-formed commands that the executor accepts.
struct MenuEntry {
    const char* type;
    const char* params_json;
};

constexpr MenuEntry kMenu[] = {
    {"create_box",      "{\"width\":10,\"height\":5,\"depth\":2}"},
    {"create_cylinder", "{\"radius\":2,\"height\":4}"},
    {"create_sphere",   "{\"radius\":3}"},
    {"create_cone",     "{\"radius1\":3,\"radius2\":1,\"height\":4}"},
    {"create_torus",    "{\"major_radius\":5,\"minor_radius\":1}"},
};

bool valid_history_change(int before, int after, int delta_min, int delta_max) {
    return (after - before) >= delta_min && (after - before) <= delta_max;
}

}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    CaideContext ctx = nullptr;
    if (caide_context_create(&ctx) != CAIDE_OK) {
        return 0;
    }
    CaideDocument doc = nullptr;
    if (caide_document_create(ctx, &doc) != CAIDE_OK) {
        caide_context_destroy(ctx);
        return 0;
    }

    int prev_count = caide_document_history_count(doc);
    if (prev_count != 0) {
        __builtin_trap();
    }

    size_t ops = (size > kMaxOps) ? kMaxOps : size;
    for (size_t i = 0; i < ops; ++i) {
        uint8_t op = data[i];
        int before = caide_document_history_count(doc);
        if (before < 0) {
            __builtin_trap();
        }

        switch (op % 4) {
            case 0:
            case 1: {
                // Execute a well-formed command from the menu.
                const MenuEntry& entry = kMenu[(op >> 2) % (sizeof(kMenu)/sizeof(kMenu[0]))];
                CaideCommand cmd;
                cmd.type = entry.type;
                cmd.params_json = entry.params_json;
                cmd.target_ref = nullptr;
                CaideShape result = nullptr;
                CaideError code = caide_command_execute(doc, &cmd, &result);
                int after = caide_document_history_count(doc);
                if (code == CAIDE_OK) {
                    if (!valid_history_change(before, after, 1, 1)) {
                        __builtin_trap();
                    }
                    if (result != nullptr) {
                        caide_shape_destroy(result);
                    }
                } else {
                    // No state change on failure.
                    if (after != before) {
                        __builtin_trap();
                    }
                }
                break;
            }
            case 2: {
                CaideError code = caide_document_undo(doc);
                int after = caide_document_history_count(doc);
                if (code == CAIDE_OK) {
                    if (!valid_history_change(before, after, -1, -1)) {
                        __builtin_trap();
                    }
                } else if (code == CAIDE_ERR_HISTORY_EMPTY) {
                    if (after != before || before != 0) {
                        __builtin_trap();
                    }
                } else {
                    __builtin_trap();
                }
                break;
            }
            case 3: {
                CaideError code = caide_document_redo(doc);
                int after = caide_document_history_count(doc);
                if (code == CAIDE_OK) {
                    if (!valid_history_change(before, after, 1, 1)) {
                        __builtin_trap();
                    }
                } else if (code == CAIDE_ERR_HISTORY_EMPTY) {
                    if (after != before) {
                        __builtin_trap();
                    }
                } else {
                    __builtin_trap();
                }
                break;
            }
        }
    }

    // Final invariant: undo + redo back-to-back must restore history_count.
    int base = caide_document_history_count(doc);
    if (base > 0) {
        if (caide_document_undo(doc) == CAIDE_OK) {
            int after_undo = caide_document_history_count(doc);
            if (after_undo != base - 1) {
                __builtin_trap();
            }
            if (caide_document_redo(doc) == CAIDE_OK) {
                int after_redo = caide_document_history_count(doc);
                if (after_redo != base) {
                    __builtin_trap();
                }
            }
        }
    }

    caide_document_destroy(doc);
    caide_context_destroy(ctx);
    return 0;
}

#include "standalone_main.inc"
