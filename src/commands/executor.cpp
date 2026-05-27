#include "executor.h"

#include "history.h"
#include "../geometry/advanced.h"
#include "../geometry/booleans.h"
#include "../geometry/features.h"
#include "../geometry/primitives.h"
#include "../geometry/transforms.h"

namespace caide::commands {
namespace {

struct ParsedJson {
    std::unordered_map<std::string, std::string> values;
};

bool parse_object(const std::string& json, ParsedJson& out) {
    std::string text = trim_copy(json);
    if (text.empty()) {
        return true;
    }
    if (text.front() != '{' || text.back() != '}') {
        return false;
    }
    text = trim_copy(text.substr(1, text.size() - 2));
    if (text.empty()) {
        return true;
    }
    for (const std::string& part : split_csv(text)) {
        const std::size_t colon = part.find(':');
        if (colon == std::string::npos) {
            return false;
        }
        std::string key = trim_copy(part.substr(0, colon));
        std::string value = trim_copy(part.substr(colon + 1));
        if (key.size() < 2 || key.front() != '"' || key.back() != '"') {
            return false;
        }
        key = key.substr(1, key.size() - 2);
        out.values[key] = value;
    }
    return true;
}

std::optional<double> parse_number(const ParsedJson& json, const std::string& key) {
    const auto it = json.values.find(key);
    if (it == json.values.end()) {
        return std::nullopt;
    }
    try {
        return std::stod(it->second);
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<std::string> parse_string(const ParsedJson& json, const std::string& key) {
    const auto it = json.values.find(key);
    if (it == json.values.end()) {
        return std::nullopt;
    }
    std::string value = trim_copy(it->second);
    if (value == "null") {
        return std::nullopt;
    }
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
        return value.substr(1, value.size() - 2);
    }
    return value;
}

std::vector<int> parse_int_array(const ParsedJson& json, const std::string& key) {
    const auto it = json.values.find(key);
    if (it == json.values.end()) {
        return {};
    }
    std::string value = trim_copy(it->second);
    if (value.size() < 2 || value.front() != '[' || value.back() != ']') {
        return {};
    }
    std::vector<int> out;
    for (const std::string& part : split_csv(value.substr(1, value.size() - 2))) {
        if (part.empty()) {
            continue;
        }
        out.push_back(std::stoi(part));
    }
    return out;
}

std::vector<std::string> parse_string_array(const ParsedJson& json, const std::string& key) {
    const auto it = json.values.find(key);
    if (it == json.values.end()) {
        return {};
    }
    std::string value = trim_copy(it->second);
    if (value.size() < 2 || value.front() != '[' || value.back() != ']') {
        return {};
    }
    std::vector<std::string> out;
    for (const std::string& part : split_csv(value.substr(1, value.size() - 2))) {
        std::string item = trim_copy(part);
        if (item.size() >= 2 && item.front() == '"' && item.back() == '"') {
            out.push_back(item.substr(1, item.size() - 2));
        } else if (!item.empty()) {
            out.push_back(item);
        }
    }
    return out;
}

std::shared_ptr<ShapeData> register_result(DocumentData& doc, const std::shared_ptr<ShapeData>& shape, const CaideCommand& command) {
    assign_reference(doc, shape, "cmd");
    if (shape->label.empty()) {
        shape->label = shape->reference_id;
    }
    push_history(doc, command.type ? command.type : "", command.params_json ? command.params_json : "{}", command.target_ref ? command.target_ref : "", shape);
    return shape;
}

Result<std::shared_ptr<ShapeData>> resolve(DocumentData& doc, const std::string& ref) {
    auto result = resolve_reference(doc, ref);
    if (!result) {
        return result;
    }
    return result;
}

}  // namespace

Status validate_command(const CaideCommand& command) {
    if (command.type == nullptr) {
        return set_error(CAIDE_ERR_COMMAND_INVALID, "command.type is required");
    }
    const std::string type = lowercase_copy(command.type);
    static const std::vector<std::string> supported = {
        "create_box", "create_cylinder", "create_sphere", "create_cone", "create_torus",
        "boolean_union", "boolean_subtract", "boolean_intersect",
        "translate", "rotate", "scale", "mirror",
        "fillet", "chamfer", "shell", "draft",
        "extrude", "revolve", "sweep", "loft"
    };
    if (std::find(supported.begin(), supported.end(), type) == supported.end()) {
        return set_error(CAIDE_ERR_COMMAND_INVALID, "unsupported command type: " + type);
    }
    if (command.params_json != nullptr) {
        ParsedJson parsed;
        if (!parse_object(command.params_json, parsed)) {
            return set_error(CAIDE_ERR_COMMAND_INVALID, "params_json must be a flat JSON object");
        }
    }
    clear_error();
    return Status::ok();
}

Result<std::shared_ptr<ShapeData>> execute_command(DocumentData& doc, const CaideCommand& command) {
    if (auto status = validate_command(command); !status) {
        return {status, nullptr};
    }

    const std::string type = lowercase_copy(command.type);
    ParsedJson json;
    if (command.params_json != nullptr && !parse_object(command.params_json, json)) {
        return {set_error(CAIDE_ERR_COMMAND_INVALID, "unable to parse params_json"), nullptr};
    }

    if (type == "create_box") {
        auto width = parse_number(json, "width");
        auto height = parse_number(json, "height");
        auto depth = parse_number(json, "depth");
        if (!width || !height || !depth) {
            return {set_error(CAIDE_ERR_COMMAND_INVALID, "create_box requires width, height, depth"), nullptr};
        }
        auto result = geometry::create_box(doc, *width, *height, *depth);
        return result;
    }
    if (type == "create_cylinder") {
        auto radius = parse_number(json, "radius");
        auto height = parse_number(json, "height");
        if (!radius || !height) {
            return {set_error(CAIDE_ERR_COMMAND_INVALID, "create_cylinder requires radius and height"), nullptr};
        }
        return geometry::create_cylinder(doc, *radius, *height);
    }
    if (type == "create_sphere") {
        auto radius = parse_number(json, "radius");
        if (!radius) {
            return {set_error(CAIDE_ERR_COMMAND_INVALID, "create_sphere requires radius"), nullptr};
        }
        return geometry::create_sphere(doc, *radius);
    }
    if (type == "create_cone") {
        auto radius1 = parse_number(json, "radius1");
        auto radius2 = parse_number(json, "radius2");
        auto height = parse_number(json, "height");
        if (!radius1 || !radius2 || !height) {
            return {set_error(CAIDE_ERR_COMMAND_INVALID, "create_cone requires radius1, radius2, height"), nullptr};
        }
        return geometry::create_cone(doc, *radius1, *radius2, *height);
    }
    if (type == "create_torus") {
        auto major = parse_number(json, "major_radius");
        auto minor = parse_number(json, "minor_radius");
        if (!major || !minor) {
            return {set_error(CAIDE_ERR_COMMAND_INVALID, "create_torus requires major_radius and minor_radius"), nullptr};
        }
        return geometry::create_torus(doc, *major, *minor);
    }

    const std::string primary_ref = command.target_ref != nullptr ? command.target_ref : parse_string(json, "target_ref").value_or("");
    if (primary_ref.empty() && type != "loft" && type != "boolean_union" && type != "boolean_subtract" && type != "boolean_intersect" && type != "sweep") {
        return {set_error(CAIDE_ERR_COMMAND_INVALID, "target_ref is required for this command"), nullptr};
    }

    if (type == "boolean_union" || type == "boolean_subtract" || type == "boolean_intersect") {
        const std::string ref_a = command.target_ref != nullptr ? command.target_ref : parse_string(json, "ref_a").value_or("");
        const std::string ref_b = parse_string(json, "ref_b").value_or("");
        if (ref_a.empty() || ref_b.empty()) {
            return {set_error(CAIDE_ERR_COMMAND_INVALID, "boolean commands require ref_a/target_ref and ref_b"), nullptr};
        }
        auto a = resolve(doc, ref_a);
        auto b = resolve(doc, ref_b);
        if (!a || !b) {
            return {!a ? a.status : b.status, nullptr};
        }
        Result<std::shared_ptr<ShapeData>> result;
        if (type == "boolean_union") {
            result = geometry::boolean_union(a.value, b.value);
        } else if (type == "boolean_subtract") {
            result = geometry::boolean_subtract(a.value, b.value);
        } else {
            result = geometry::boolean_intersect(a.value, b.value);
        }
        if (!result) {
            return result;
        }
        register_result(doc, result.value, command);
        clear_error();
        return result;
    }

    if (type == "loft") {
        std::vector<std::shared_ptr<ShapeData>> shapes;
        for (const std::string& ref : parse_string_array(json, "profile_refs")) {
            auto shape = resolve(doc, ref);
            if (!shape) {
                return {shape.status, nullptr};
            }
            shapes.push_back(shape.value);
        }
        auto result = geometry::loft(shapes);
        if (!result) {
            return result;
        }
        register_result(doc, result.value, command);
        return result;
    }

    if (type == "sweep") {
        const std::string profile_ref = command.target_ref != nullptr ? command.target_ref : parse_string(json, "profile_ref").value_or("");
        const std::string spine_ref = parse_string(json, "spine_ref").value_or("");
        if (profile_ref.empty() || spine_ref.empty()) {
            return {set_error(CAIDE_ERR_COMMAND_INVALID, "sweep requires profile_ref/target_ref and spine_ref"), nullptr};
        }
        auto profile = resolve(doc, profile_ref);
        auto spine = resolve(doc, spine_ref);
        if (!profile || !spine) {
            return {!profile ? profile.status : spine.status, nullptr};
        }
        auto result = geometry::sweep(profile.value, spine.value);
        if (!result) {
            return result;
        }
        register_result(doc, result.value, command);
        return result;
    }

    auto target = resolve(doc, primary_ref);
    if (!target) {
        return {target.status, nullptr};
    }

    Result<std::shared_ptr<ShapeData>> result;
    if (type == "translate") {
        auto dx = parse_number(json, "dx");
        auto dy = parse_number(json, "dy");
        auto dz = parse_number(json, "dz");
        if (!dx || !dy || !dz) {
            return {set_error(CAIDE_ERR_COMMAND_INVALID, "translate requires dx, dy, dz"), nullptr};
        }
        result = geometry::translate(target.value, *dx, *dy, *dz);
    } else if (type == "rotate") {
        auto axis_x = parse_number(json, "axis_x");
        auto axis_y = parse_number(json, "axis_y");
        auto axis_z = parse_number(json, "axis_z");
        auto angle = parse_number(json, "angle_deg");
        if (!axis_x || !axis_y || !axis_z || !angle) {
            return {set_error(CAIDE_ERR_COMMAND_INVALID, "rotate requires axis_x, axis_y, axis_z, angle_deg"), nullptr};
        }
        result = geometry::rotate(target.value, *axis_x, *axis_y, *axis_z, *angle);
    } else if (type == "scale") {
        auto cx = parse_number(json, "cx");
        auto cy = parse_number(json, "cy");
        auto cz = parse_number(json, "cz");
        auto factor = parse_number(json, "factor");
        if (!cx || !cy || !cz || !factor) {
            return {set_error(CAIDE_ERR_COMMAND_INVALID, "scale requires cx, cy, cz, factor"), nullptr};
        }
        result = geometry::scale(target.value, *cx, *cy, *cz, *factor);
    } else if (type == "mirror") {
        auto plane_x = parse_number(json, "plane_x");
        auto plane_y = parse_number(json, "plane_y");
        auto plane_z = parse_number(json, "plane_z");
        auto plane_d = parse_number(json, "plane_d");
        if (!plane_x || !plane_y || !plane_z || !plane_d) {
            return {set_error(CAIDE_ERR_COMMAND_INVALID, "mirror requires plane_x, plane_y, plane_z, plane_d"), nullptr};
        }
        result = geometry::mirror(target.value, *plane_x, *plane_y, *plane_z, *plane_d);
    } else if (type == "fillet") {
        auto radius = parse_number(json, "radius");
        if (!radius) {
            return {set_error(CAIDE_ERR_COMMAND_INVALID, "fillet requires radius"), nullptr};
        }
        result = geometry::fillet(target.value, *radius, parse_int_array(json, "edge_indices"));
    } else if (type == "chamfer") {
        auto distance = parse_number(json, "distance");
        if (!distance) {
            return {set_error(CAIDE_ERR_COMMAND_INVALID, "chamfer requires distance"), nullptr};
        }
        result = geometry::chamfer(target.value, *distance, parse_int_array(json, "edge_indices"));
    } else if (type == "shell") {
        auto thickness = parse_number(json, "thickness");
        if (!thickness) {
            return {set_error(CAIDE_ERR_COMMAND_INVALID, "shell requires thickness"), nullptr};
        }
        result = geometry::shell(target.value, *thickness, parse_int_array(json, "face_indices"));
    } else if (type == "draft") {
        auto angle = parse_number(json, "angle_deg");
        auto dir_x = parse_number(json, "dir_x");
        auto dir_y = parse_number(json, "dir_y");
        auto dir_z = parse_number(json, "dir_z");
        if (!angle || !dir_x || !dir_y || !dir_z) {
            return {set_error(CAIDE_ERR_COMMAND_INVALID, "draft requires angle_deg, dir_x, dir_y, dir_z"), nullptr};
        }
        result = geometry::draft(target.value, *angle, {*dir_x, *dir_y, *dir_z}, parse_int_array(json, "face_indices"));
    } else if (type == "extrude") {
        auto dx = parse_number(json, "dx");
        auto dy = parse_number(json, "dy");
        auto dz = parse_number(json, "dz");
        if (!dx || !dy || !dz) {
            return {set_error(CAIDE_ERR_COMMAND_INVALID, "extrude requires dx, dy, dz"), nullptr};
        }
        result = geometry::extrude(target.value, *dx, *dy, *dz);
    } else if (type == "revolve") {
        auto axis_x = parse_number(json, "axis_x");
        auto axis_y = parse_number(json, "axis_y");
        auto axis_z = parse_number(json, "axis_z");
        auto angle = parse_number(json, "angle_deg");
        if (!axis_x || !axis_y || !axis_z || !angle) {
            return {set_error(CAIDE_ERR_COMMAND_INVALID, "revolve requires axis_x, axis_y, axis_z, angle_deg"), nullptr};
        }
        result = geometry::revolve(target.value, *axis_x, *axis_y, *axis_z, *angle);
    } else {
        return {set_error(CAIDE_ERR_NOT_IMPLEMENTED, "command not implemented: " + type), nullptr};
    }

    if (!result) {
        return result;
    }
    register_result(doc, result.value, command);
    clear_error();
    return result;
}

Status execute_batch(DocumentData& doc, const std::vector<CaideCommand>& commands) {
    for (const CaideCommand& command : commands) {
        auto result = execute_command(doc, command);
        if (!result) {
            return result.status;
        }
    }
    clear_error();
    return Status::ok();
}

}  // namespace caide::commands
