#include "transforms.h"

#ifndef CAIDE_STUB_MODE
#include <BRepBuilderAPI_Transform.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#endif

namespace caide::geometry {

Result<std::shared_ptr<ShapeData>> translate(const std::shared_ptr<ShapeData>& shape, double dx, double dy, double dz) {
    auto clone = clone_with_transform(shape, Matrix4::translation({dx, dy, dz}), "translate");
    if (!clone) {
        return clone;
    }
    clone.value->kind = ShapeKind::Transform;
    clone.value->metadata["transform"] = "translate";
    clone.value->params = {dx, dy, dz};
#ifndef CAIDE_STUB_MODE
    gp_Trsf trsf;
    trsf.SetTranslation(gp_Vec(dx, dy, dz));
    clone.value->native_shape = BRepBuilderAPI_Transform(shape->native_shape, trsf, true).Shape();
#endif
    clear_error();
    return clone;
}

Result<std::shared_ptr<ShapeData>> rotate(const std::shared_ptr<ShapeData>& shape, double axis_x, double axis_y, double axis_z, double angle_deg) {
    if (nearly_equal(length({axis_x, axis_y, axis_z}), 0.0)) {
        return {set_error(CAIDE_ERR_INVALID_PARAM, "rotation axis cannot be zero"), nullptr};
    }
    auto clone = clone_with_transform(shape, Matrix4::rotation({axis_x, axis_y, axis_z}, angle_deg), "rotate");
    if (!clone) {
        return clone;
    }
    clone.value->kind = ShapeKind::Transform;
    clone.value->metadata["transform"] = "rotate";
    clone.value->params = {axis_x, axis_y, axis_z, angle_deg};
#ifndef CAIDE_STUB_MODE
    gp_Trsf trsf;
    trsf.SetRotation(gp_Ax1(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(axis_x, axis_y, axis_z)), angle_deg * kPi / 180.0);
    clone.value->native_shape = BRepBuilderAPI_Transform(shape->native_shape, trsf, true).Shape();
#endif
    clear_error();
    return clone;
}

Result<std::shared_ptr<ShapeData>> scale(const std::shared_ptr<ShapeData>& shape, double cx, double cy, double cz, double factor) {
    if (auto status = validate_positive("factor", factor); !status) {
        return {set_error(status.code, status.message), nullptr};
    }
    auto clone = clone_with_transform(shape, Matrix4::scale({cx, cy, cz}, factor), "scale");
    if (!clone) {
        return clone;
    }
    clone.value->kind = ShapeKind::Transform;
    clone.value->metadata["transform"] = "scale";
    clone.value->params = {cx, cy, cz, factor};
    clone.value->volume = shape->volume * factor * factor * factor;
    clone.value->surface_area = shape->surface_area * factor * factor;
#ifndef CAIDE_STUB_MODE
    gp_Trsf trsf;
    trsf.SetScale(gp_Pnt(cx, cy, cz), factor);
    clone.value->native_shape = BRepBuilderAPI_Transform(shape->native_shape, trsf, true).Shape();
#endif
    clear_error();
    return clone;
}

Result<std::shared_ptr<ShapeData>> mirror(const std::shared_ptr<ShapeData>& shape, double plane_x, double plane_y, double plane_z, double plane_d) {
    if (nearly_equal(length({plane_x, plane_y, plane_z}), 0.0)) {
        return {set_error(CAIDE_ERR_INVALID_PARAM, "mirror plane normal cannot be zero"), nullptr};
    }
    auto clone = clone_with_transform(shape, Matrix4::mirror({plane_x, plane_y, plane_z}, plane_d), "mirror");
    if (!clone) {
        return clone;
    }
    clone.value->kind = ShapeKind::Transform;
    clone.value->metadata["transform"] = "mirror";
    clone.value->params = {plane_x, plane_y, plane_z, plane_d};
#ifndef CAIDE_STUB_MODE
    gp_Trsf trsf;
    trsf.SetMirror(gp_Ax2(gp_Pnt(plane_x * plane_d, plane_y * plane_d, plane_z * plane_d), gp_Dir(plane_x, plane_y, plane_z)));
    clone.value->native_shape = BRepBuilderAPI_Transform(shape->native_shape, trsf, true).Shape();
#endif
    clear_error();
    return clone;
}

}  // namespace caide::geometry
