#include <iostream>

#include "../src/commands/executor.h"
#include "../src/commands/history.h"
#include "../src/error.h"

#define CHECK(expr) do { if (!(expr)) { std::cerr << "CHECK failed: " #expr << " line=" << __LINE__ << '\n'; return 1; } } while (0)

int main() {
    caide::DocumentData doc;

    CaideCommand create_box {"create_box", "{\"width\":10,\"height\":5,\"depth\":2}", nullptr};
    auto box = caide::commands::execute_command(doc, create_box);
    CHECK(box);
    CHECK(caide::commands::history_count(doc) == 1);

    CaideCommand translate {"translate", "{\"dx\":1,\"dy\":2,\"dz\":3}", box.value->reference_id.c_str()};
    auto moved = caide::commands::execute_command(doc, translate);
    CHECK(moved);
    CHECK(caide::commands::history_count(doc) == 2);

    std::vector<CaideCommand> batch {
        {"create_cylinder", "{\"radius\":2,\"height\":4}", nullptr},
        {"create_sphere", "{\"radius\":3}", nullptr},
    };
    CHECK(caide::commands::execute_batch(doc, batch));
    CHECK(caide::commands::history_count(doc) == 4);

    const std::string other_ref = doc.history[2].result->reference_id;
    std::string boolean_json = std::string("{\"ref_b\":\"") + other_ref + "\"}";
    CaideCommand fuse {"boolean_union", boolean_json.c_str(), moved.value->reference_id.c_str()};
    auto fused = caide::commands::execute_command(doc, fuse);
    CHECK(fused);
    CHECK(caide::commands::history_count(doc) == 5);

    CHECK(caide::commands::undo(doc));
    CHECK(caide::commands::history_count(doc) == 4);
    CHECK(caide::commands::redo(doc));
    CHECK(caide::commands::history_count(doc) == 5);

    CaideCommand invalid {"unknown", "{}", nullptr};
    CHECK(!caide::commands::validate_command(invalid));

    CaideCommand bad_json {"create_box", "[]", nullptr};
    CHECK(!caide::commands::validate_command(bad_json));
    CHECK(caide::last_error_code() == CAIDE_ERR_COMMAND_INVALID);
    return 0;
}
