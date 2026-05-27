#include "../geometry/advanced.h"
#include "../geometry/booleans.h"
#include "../geometry/features.h"
#include "../geometry/primitives.h"
#include "../geometry/sketch.h"
#include "../geometry/transforms.h"
#include "../internal.h"

extern "C" {

CaideError caide_shape_create_box(CaideDocument doc, double width, double height, double depth, CaideShape* out_shape) {
    if (doc == nullptr || doc->impl == nullptr) {
        return caide::set_error(CAIDE_ERR_NULL_HANDLE, "document is null").code;
    }
    if (out_shape == nullptr) {
        return caide::set_error(CAIDE_ERR_INVALID_PARAM, "out_shape cannot be null").code;
    }
    auto result = caide::geometry::create_box(*doc->impl, width, height, depth);
    if (!result) {
        return result.status.code;
    }
    *out_shape = caide::make_shape_handle(result.value);
    return CAIDE_OK;
}

CaideError caide_shape_create_cylinder(CaideDocument doc, double radius, double height, CaideShape* out_shape) {
    if (doc == nullptr || doc->impl == nullptr || out_shape == nullptr) {
        return caide::set_error(doc == nullptr ? CAIDE_ERR_NULL_HANDLE : CAIDE_ERR_INVALID_PARAM, "invalid cylinder arguments").code;
    }
    auto result = caide::geometry::create_cylinder(*doc->impl, radius, height);
    if (!result) {
        return result.status.code;
    }
    *out_shape = caide::make_shape_handle(result.value);
    return CAIDE_OK;
}

CaideError caide_shape_create_sphere(CaideDocument doc, double radius, CaideShape* out_shape) {
    if (doc == nullptr || doc->impl == nullptr || out_shape == nullptr) {
        return caide::set_error(doc == nullptr ? CAIDE_ERR_NULL_HANDLE : CAIDE_ERR_INVALID_PARAM, "invalid sphere arguments").code;
    }
    auto result = caide::geometry::create_sphere(*doc->impl, radius);
    if (!result) {
        return result.status.code;
    }
    *out_shape = caide::make_shape_handle(result.value);
    return CAIDE_OK;
}

CaideError caide_shape_create_cone(CaideDocument doc, double radius1, double radius2, double height, CaideShape* out_shape) {
    if (doc == nullptr || doc->impl == nullptr || out_shape == nullptr) {
        return caide::set_error(doc == nullptr ? CAIDE_ERR_NULL_HANDLE : CAIDE_ERR_INVALID_PARAM, "invalid cone arguments").code;
    }
    auto result = caide::geometry::create_cone(*doc->impl, radius1, radius2, height);
    if (!result) {
        return result.status.code;
    }
    *out_shape = caide::make_shape_handle(result.value);
    return CAIDE_OK;
}

CaideError caide_shape_create_torus(CaideDocument doc, double major_radius, double minor_radius, CaideShape* out_shape) {
    if (doc == nullptr || doc->impl == nullptr || out_shape == nullptr) {
        return caide::set_error(doc == nullptr ? CAIDE_ERR_NULL_HANDLE : CAIDE_ERR_INVALID_PARAM, "invalid torus arguments").code;
    }
    auto result = caide::geometry::create_torus(*doc->impl, major_radius, minor_radius);
    if (!result) {
        return result.status.code;
    }
    *out_shape = caide::make_shape_handle(result.value);
    return CAIDE_OK;
}

void caide_shape_destroy(CaideShape shape) {
    delete shape;
    caide::clear_error();
}

CaideError caide_shape_clone(CaideShape shape, CaideShape* out_clone) {
    if (shape == nullptr || shape->impl == nullptr) {
        return caide::set_error(CAIDE_ERR_NULL_HANDLE, "shape is null").code;
    }
    if (out_clone == nullptr) {
        return caide::set_error(CAIDE_ERR_INVALID_PARAM, "out_clone cannot be null").code;
    }
    *out_clone = caide::make_shape_handle(caide::clone_shape_data(shape->impl));
    caide::clear_error();
    return CAIDE_OK;
}

int caide_shape_is_valid(CaideShape shape) {
    if (shape == nullptr || shape->impl == nullptr) {
        caide::set_error(CAIDE_ERR_NULL_HANDLE, "shape is null");
        return 0;
    }
    caide::clear_error();
    return shape->impl->valid ? 1 : 0;
}

CaideError caide_shape_bounding_box(CaideShape shape,
    double* out_min_x, double* out_min_y, double* out_min_z,
    double* out_max_x, double* out_max_y, double* out_max_z) {
    if (shape == nullptr || shape->impl == nullptr) {
        return caide::set_error(CAIDE_ERR_NULL_HANDLE, "shape is null").code;
    }
    if (out_min_x == nullptr || out_min_y == nullptr || out_min_z == nullptr || out_max_x == nullptr || out_max_y == nullptr || out_max_z == nullptr) {
        return caide::set_error(CAIDE_ERR_INVALID_PARAM, "all bounding box outputs are required").code;
    }
    *out_min_x = shape->impl->bbox.min.x;
    *out_min_y = shape->impl->bbox.min.y;
    *out_min_z = shape->impl->bbox.min.z;
    *out_max_x = shape->impl->bbox.max.x;
    *out_max_y = shape->impl->bbox.max.y;
    *out_max_z = shape->impl->bbox.max.z;
    caide::clear_error();
    return CAIDE_OK;
}

CaideError caide_shape_volume(CaideShape shape, double* out_volume) {
    if (shape == nullptr || shape->impl == nullptr) {
        return caide::set_error(CAIDE_ERR_NULL_HANDLE, "shape is null").code;
    }
    if (out_volume == nullptr) {
        return caide::set_error(CAIDE_ERR_INVALID_PARAM, "out_volume cannot be null").code;
    }
    *out_volume = shape->impl->volume;
    caide::clear_error();
    return CAIDE_OK;
}

CaideError caide_shape_surface_area(CaideShape shape, double* out_area) {
    if (shape == nullptr || shape->impl == nullptr) {
        return caide::set_error(CAIDE_ERR_NULL_HANDLE, "shape is null").code;
    }
    if (out_area == nullptr) {
        return caide::set_error(CAIDE_ERR_INVALID_PARAM, "out_area cannot be null").code;
    }
    *out_area = shape->impl->surface_area;
    caide::clear_error();
    return CAIDE_OK;
}

CaideError caide_boolean_union(CaideShape shape_a, CaideShape shape_b, CaideShape* out_result) {
    if (shape_a == nullptr || shape_b == nullptr || out_result == nullptr) {
        return caide::set_error(shape_a == nullptr || shape_b == nullptr ? CAIDE_ERR_NULL_HANDLE : CAIDE_ERR_INVALID_PARAM, "invalid union arguments").code;
    }
    auto result = caide::geometry::boolean_union(shape_a->impl, shape_b->impl);
    if (!result) {
        return result.status.code;
    }
    *out_result = caide::make_shape_handle(result.value);
    return CAIDE_OK;
}

CaideError caide_boolean_subtract(CaideShape shape_a, CaideShape shape_b, CaideShape* out_result) {
    if (shape_a == nullptr || shape_b == nullptr || out_result == nullptr) {
        return caide::set_error(shape_a == nullptr || shape_b == nullptr ? CAIDE_ERR_NULL_HANDLE : CAIDE_ERR_INVALID_PARAM, "invalid subtract arguments").code;
    }
    auto result = caide::geometry::boolean_subtract(shape_a->impl, shape_b->impl);
    if (!result) {
        return result.status.code;
    }
    *out_result = caide::make_shape_handle(result.value);
    return CAIDE_OK;
}

CaideError caide_boolean_intersect(CaideShape shape_a, CaideShape shape_b, CaideShape* out_result) {
    if (shape_a == nullptr || shape_b == nullptr || out_result == nullptr) {
        return caide::set_error(shape_a == nullptr || shape_b == nullptr ? CAIDE_ERR_NULL_HANDLE : CAIDE_ERR_INVALID_PARAM, "invalid intersect arguments").code;
    }
    auto result = caide::geometry::boolean_intersect(shape_a->impl, shape_b->impl);
    if (!result) {
        return result.status.code;
    }
    *out_result = caide::make_shape_handle(result.value);
    return CAIDE_OK;
}

CaideError caide_transform_translate(CaideShape shape, double dx, double dy, double dz, CaideShape* out_result) {
    if (shape == nullptr || out_result == nullptr) {
        return caide::set_error(shape == nullptr ? CAIDE_ERR_NULL_HANDLE : CAIDE_ERR_INVALID_PARAM, "invalid translate arguments").code;
    }
    auto result = caide::geometry::translate(shape->impl, dx, dy, dz);
    if (!result) {
        return result.status.code;
    }
    *out_result = caide::make_shape_handle(result.value);
    return CAIDE_OK;
}

CaideError caide_transform_rotate(CaideShape shape, double axis_x, double axis_y, double axis_z, double angle_deg, CaideShape* out_result) {
    if (shape == nullptr || out_result == nullptr) {
        return caide::set_error(shape == nullptr ? CAIDE_ERR_NULL_HANDLE : CAIDE_ERR_INVALID_PARAM, "invalid rotate arguments").code;
    }
    auto result = caide::geometry::rotate(shape->impl, axis_x, axis_y, axis_z, angle_deg);
    if (!result) {
        return result.status.code;
    }
    *out_result = caide::make_shape_handle(result.value);
    return CAIDE_OK;
}

CaideError caide_transform_scale(CaideShape shape, double cx, double cy, double cz, double factor, CaideShape* out_result) {
    if (shape == nullptr || out_result == nullptr) {
        return caide::set_error(shape == nullptr ? CAIDE_ERR_NULL_HANDLE : CAIDE_ERR_INVALID_PARAM, "invalid scale arguments").code;
    }
    auto result = caide::geometry::scale(shape->impl, cx, cy, cz, factor);
    if (!result) {
        return result.status.code;
    }
    *out_result = caide::make_shape_handle(result.value);
    return CAIDE_OK;
}

CaideError caide_transform_mirror(CaideShape shape, double plane_x, double plane_y, double plane_z, double plane_d, CaideShape* out_result) {
    if (shape == nullptr || out_result == nullptr) {
        return caide::set_error(shape == nullptr ? CAIDE_ERR_NULL_HANDLE : CAIDE_ERR_INVALID_PARAM, "invalid mirror arguments").code;
    }
    auto result = caide::geometry::mirror(shape->impl, plane_x, plane_y, plane_z, plane_d);
    if (!result) {
        return result.status.code;
    }
    *out_result = caide::make_shape_handle(result.value);
    return CAIDE_OK;
}

CaideError caide_feature_fillet(CaideShape shape, double radius, const int* edge_indices, int edge_count, CaideShape* out_result) {
    if (shape == nullptr || out_result == nullptr) {
        return caide::set_error(shape == nullptr ? CAIDE_ERR_NULL_HANDLE : CAIDE_ERR_INVALID_PARAM, "invalid fillet arguments").code;
    }
    std::vector<int> edges;
    if (edge_indices != nullptr && edge_count > 0) {
        edges.assign(edge_indices, edge_indices + edge_count);
    }
    auto result = caide::geometry::fillet(shape->impl, radius, edges);
    if (!result) {
        return result.status.code;
    }
    *out_result = caide::make_shape_handle(result.value);
    return CAIDE_OK;
}

CaideError caide_feature_chamfer(CaideShape shape, double distance, const int* edge_indices, int edge_count, CaideShape* out_result) {
    if (shape == nullptr || out_result == nullptr) {
        return caide::set_error(shape == nullptr ? CAIDE_ERR_NULL_HANDLE : CAIDE_ERR_INVALID_PARAM, "invalid chamfer arguments").code;
    }
    std::vector<int> edges;
    if (edge_indices != nullptr && edge_count > 0) {
        edges.assign(edge_indices, edge_indices + edge_count);
    }
    auto result = caide::geometry::chamfer(shape->impl, distance, edges);
    if (!result) {
        return result.status.code;
    }
    *out_result = caide::make_shape_handle(result.value);
    return CAIDE_OK;
}

CaideError caide_feature_shell(CaideShape shape, double thickness, const int* face_indices, int face_count, CaideShape* out_result) {
    if (shape == nullptr || out_result == nullptr) {
        return caide::set_error(shape == nullptr ? CAIDE_ERR_NULL_HANDLE : CAIDE_ERR_INVALID_PARAM, "invalid shell arguments").code;
    }
    std::vector<int> faces;
    if (face_indices != nullptr && face_count > 0) {
        faces.assign(face_indices, face_indices + face_count);
    }
    auto result = caide::geometry::shell(shape->impl, thickness, faces);
    if (!result) {
        return result.status.code;
    }
    *out_result = caide::make_shape_handle(result.value);
    return CAIDE_OK;
}

CaideError caide_feature_draft(CaideShape shape, double angle_deg, double dir_x, double dir_y, double dir_z, const int* face_indices, int face_count, CaideShape* out_result) {
    if (shape == nullptr || out_result == nullptr) {
        return caide::set_error(shape == nullptr ? CAIDE_ERR_NULL_HANDLE : CAIDE_ERR_INVALID_PARAM, "invalid draft arguments").code;
    }
    std::vector<int> faces;
    if (face_indices != nullptr && face_count > 0) {
        faces.assign(face_indices, face_indices + face_count);
    }
    auto result = caide::geometry::draft(shape->impl, angle_deg, {dir_x, dir_y, dir_z}, faces);
    if (!result) {
        return result.status.code;
    }
    *out_result = caide::make_shape_handle(result.value);
    return CAIDE_OK;
}

CaideError caide_sketch_create(CaideDocument doc, CaideSketch* out_sketch) {
    if (doc == nullptr || doc->impl == nullptr) {
        return caide::set_error(CAIDE_ERR_NULL_HANDLE, "document is null").code;
    }
    if (out_sketch == nullptr) {
        return caide::set_error(CAIDE_ERR_INVALID_PARAM, "out_sketch cannot be null").code;
    }
    auto sketch = caide::geometry::create_sketch(doc->impl);
    if (!sketch) {
        return sketch.status.code;
    }
    *out_sketch = caide::make_sketch_handle(sketch.value);
    return CAIDE_OK;
}

void caide_sketch_destroy(CaideSketch sketch) {
    delete sketch;
    caide::clear_error();
}

CaideError caide_sketch_add_line(CaideSketch sketch, double x1, double y1, double x2, double y2) {
    if (sketch == nullptr || sketch->impl == nullptr) {
        return caide::set_error(CAIDE_ERR_NULL_HANDLE, "sketch is null").code;
    }
    return caide::geometry::add_line(*sketch->impl, x1, y1, x2, y2).code;
}

CaideError caide_sketch_add_arc(CaideSketch sketch, double cx, double cy, double radius, double start_deg, double end_deg) {
    if (sketch == nullptr || sketch->impl == nullptr) {
        return caide::set_error(CAIDE_ERR_NULL_HANDLE, "sketch is null").code;
    }
    return caide::geometry::add_arc(*sketch->impl, cx, cy, radius, start_deg, end_deg).code;
}

CaideError caide_sketch_add_circle(CaideSketch sketch, double cx, double cy, double radius) {
    if (sketch == nullptr || sketch->impl == nullptr) {
        return caide::set_error(CAIDE_ERR_NULL_HANDLE, "sketch is null").code;
    }
    return caide::geometry::add_circle(*sketch->impl, cx, cy, radius).code;
}

CaideError caide_sketch_add_spline(CaideSketch sketch, const double* points, int point_count) {
    if (sketch == nullptr || sketch->impl == nullptr) {
        return caide::set_error(CAIDE_ERR_NULL_HANDLE, "sketch is null").code;
    }
    return caide::geometry::add_spline(*sketch->impl, points, point_count).code;
}

CaideError caide_sketch_close(CaideSketch sketch) {
    if (sketch == nullptr || sketch->impl == nullptr) {
        return caide::set_error(CAIDE_ERR_NULL_HANDLE, "sketch is null").code;
    }
    return caide::geometry::close_sketch(*sketch->impl).code;
}

CaideError caide_sketch_to_face(CaideSketch sketch, CaideShape* out_face) {
    if (sketch == nullptr || sketch->impl == nullptr) {
        return caide::set_error(CAIDE_ERR_NULL_HANDLE, "sketch is null").code;
    }
    if (out_face == nullptr) {
        return caide::set_error(CAIDE_ERR_INVALID_PARAM, "out_face cannot be null").code;
    }
    auto face = caide::geometry::sketch_to_face(*sketch->impl);
    if (!face) {
        return face.status.code;
    }
    *out_face = caide::make_shape_handle(face.value);
    return CAIDE_OK;
}

CaideError caide_sketch_to_wire(CaideSketch sketch, CaideShape* out_wire) {
    if (sketch == nullptr || sketch->impl == nullptr) {
        return caide::set_error(CAIDE_ERR_NULL_HANDLE, "sketch is null").code;
    }
    if (out_wire == nullptr) {
        return caide::set_error(CAIDE_ERR_INVALID_PARAM, "out_wire cannot be null").code;
    }
    auto wire = caide::geometry::sketch_to_wire(*sketch->impl);
    if (!wire) {
        return wire.status.code;
    }
    *out_wire = caide::make_shape_handle(wire.value);
    return CAIDE_OK;
}

CaideError caide_shape_extrude(CaideShape profile, double dx, double dy, double dz, CaideShape* out_result) {
    if (profile == nullptr || out_result == nullptr) {
        return caide::set_error(profile == nullptr ? CAIDE_ERR_NULL_HANDLE : CAIDE_ERR_INVALID_PARAM, "invalid extrude arguments").code;
    }
    auto result = caide::geometry::extrude(profile->impl, dx, dy, dz);
    if (!result) {
        return result.status.code;
    }
    *out_result = caide::make_shape_handle(result.value);
    return CAIDE_OK;
}

CaideError caide_shape_revolve(CaideShape profile, double axis_x, double axis_y, double axis_z, double angle_deg, CaideShape* out_result) {
    if (profile == nullptr || out_result == nullptr) {
        return caide::set_error(profile == nullptr ? CAIDE_ERR_NULL_HANDLE : CAIDE_ERR_INVALID_PARAM, "invalid revolve arguments").code;
    }
    auto result = caide::geometry::revolve(profile->impl, axis_x, axis_y, axis_z, angle_deg);
    if (!result) {
        return result.status.code;
    }
    *out_result = caide::make_shape_handle(result.value);
    return CAIDE_OK;
}

CaideError caide_shape_sweep(CaideShape profile, CaideShape spine, CaideShape* out_result) {
    if (profile == nullptr || spine == nullptr || out_result == nullptr) {
        return caide::set_error(profile == nullptr || spine == nullptr ? CAIDE_ERR_NULL_HANDLE : CAIDE_ERR_INVALID_PARAM, "invalid sweep arguments").code;
    }
    auto result = caide::geometry::sweep(profile->impl, spine->impl);
    if (!result) {
        return result.status.code;
    }
    *out_result = caide::make_shape_handle(result.value);
    return CAIDE_OK;
}

CaideError caide_shape_loft(const CaideShape* profiles, int profile_count, CaideShape* out_result) {
    if (profiles == nullptr || out_result == nullptr) {
        return caide::set_error(CAIDE_ERR_INVALID_PARAM, "profiles and out_result are required").code;
    }
    std::vector<std::shared_ptr<caide::ShapeData>> input;
    for (int i = 0; i < profile_count; ++i) {
        if (profiles[i] == nullptr || profiles[i]->impl == nullptr) {
            return caide::set_error(CAIDE_ERR_NULL_HANDLE, "profile handle is null").code;
        }
        input.push_back(profiles[i]->impl);
    }
    auto result = caide::geometry::loft(input);
    if (!result) {
        return result.status.code;
    }
    *out_result = caide::make_shape_handle(result.value);
    return CAIDE_OK;
}

}  // extern "C"
