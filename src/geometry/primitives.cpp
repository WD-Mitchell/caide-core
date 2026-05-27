#include "primitives.h"

#ifndef CAIDE_STUB_MODE
#include <BRepBndLib.hxx>
#include <BRepGProp.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeTorus.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#endif

namespace caide::geometry {
namespace {

std::shared_ptr<ShapeData> finalize_shape(DocumentData& doc,
                                          const std::shared_ptr<ShapeData>& shape,
                                          const std::string& type,
                                          const std::string& params_json) {
    assign_reference(doc, shape, "shape");
    shape->label = shape->reference_id;
    push_history(doc, type, params_json, shape->reference_id, shape);
    return shape;
}

#ifndef CAIDE_STUB_MODE
void populate_occ_metrics(ShapeData& shape) {
    Bnd_Box bbox;
    BRepBndLib::Add(shape.native_shape, bbox);
    Standard_Real xmin = 0.0;
    Standard_Real ymin = 0.0;
    Standard_Real zmin = 0.0;
    Standard_Real xmax = 0.0;
    Standard_Real ymax = 0.0;
    Standard_Real zmax = 0.0;
    bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    shape.bbox = make_bbox({xmin, ymin, zmin}, {xmax, ymax, zmax});

    GProp_GProps properties;
    BRepGProp::VolumeProperties(shape.native_shape, properties);
    shape.volume = properties.Mass();
    BRepGProp::SurfaceProperties(shape.native_shape, properties);
    shape.surface_area = properties.Mass();
}
#endif

}  // namespace

Result<std::shared_ptr<ShapeData>> create_box(DocumentData& doc, double width, double height, double depth) {
    if (auto status = validate_positive("width", width); !status) {
        return {set_error(status.code, status.message), nullptr};
    }
    if (auto status = validate_positive("height", height); !status) {
        return {set_error(status.code, status.message), nullptr};
    }
    if (auto status = validate_positive("depth", depth); !status) {
        return {set_error(status.code, status.message), nullptr};
    }

    const BoundingBox bbox = make_bbox({0.0, 0.0, 0.0}, {width, height, depth});
    auto shape = make_shape(ShapeKind::Box, "box", bbox, width * height * depth, 2.0 * ((width * height) + (width * depth) + (height * depth)));
    shape->params = {width, height, depth};
    shape->metadata["primitive"] = "box";
#ifndef CAIDE_STUB_MODE
    shape->native_shape = BRepPrimAPI_MakeBox(width, height, depth).Shape();
    populate_occ_metrics(*shape);
#endif
    clear_error();
    return {Status::ok(), finalize_shape(doc, shape, "create_box", "{\"width\":" + format_double(width) + ",\"height\":" + format_double(height) + ",\"depth\":" + format_double(depth) + "}")};
}

Result<std::shared_ptr<ShapeData>> create_cylinder(DocumentData& doc, double radius, double height) {
    if (auto status = validate_positive("radius", radius); !status) {
        return {set_error(status.code, status.message), nullptr};
    }
    if (auto status = validate_positive("height", height); !status) {
        return {set_error(status.code, status.message), nullptr};
    }

    const BoundingBox bbox = make_bbox({-radius, -radius, 0.0}, {radius, radius, height});
    auto shape = make_shape(ShapeKind::Cylinder, "cylinder", bbox, kPi * radius * radius * height, 2.0 * kPi * radius * (height + radius));
    shape->params = {radius, height};
    shape->metadata["primitive"] = "cylinder";
#ifndef CAIDE_STUB_MODE
    shape->native_shape = BRepPrimAPI_MakeCylinder(radius, height).Shape();
    populate_occ_metrics(*shape);
#endif
    clear_error();
    return {Status::ok(), finalize_shape(doc, shape, "create_cylinder", "{\"radius\":" + format_double(radius) + ",\"height\":" + format_double(height) + "}")};
}

Result<std::shared_ptr<ShapeData>> create_sphere(DocumentData& doc, double radius) {
    if (auto status = validate_positive("radius", radius); !status) {
        return {set_error(status.code, status.message), nullptr};
    }

    const BoundingBox bbox = make_bbox({-radius, -radius, -radius}, {radius, radius, radius});
    auto shape = make_shape(ShapeKind::Sphere, "sphere", bbox, (4.0 / 3.0) * kPi * radius * radius * radius, 4.0 * kPi * radius * radius);
    shape->params = {radius};
    shape->metadata["primitive"] = "sphere";
#ifndef CAIDE_STUB_MODE
    shape->native_shape = BRepPrimAPI_MakeSphere(radius).Shape();
    populate_occ_metrics(*shape);
#endif
    clear_error();
    return {Status::ok(), finalize_shape(doc, shape, "create_sphere", "{\"radius\":" + format_double(radius) + "}")};
}

Result<std::shared_ptr<ShapeData>> create_cone(DocumentData& doc, double radius1, double radius2, double height) {
    if (auto status = validate_positive("radius1", radius1); !status) {
        return {set_error(status.code, status.message), nullptr};
    }
    if (auto status = validate_positive("radius2", radius2); !status) {
        return {set_error(status.code, status.message), nullptr};
    }
    if (auto status = validate_positive("height", height); !status) {
        return {set_error(status.code, status.message), nullptr};
    }

    const double max_radius = std::max(radius1, radius2);
    const double slant = std::sqrt(((radius1 - radius2) * (radius1 - radius2)) + (height * height));
    const double volume = (kPi * height * ((radius1 * radius1) + (radius1 * radius2) + (radius2 * radius2))) / 3.0;
    const double area = (kPi * (radius1 + radius2) * slant) + (kPi * radius1 * radius1) + (kPi * radius2 * radius2);
    const BoundingBox bbox = make_bbox({-max_radius, -max_radius, 0.0}, {max_radius, max_radius, height});
    auto shape = make_shape(ShapeKind::Cone, "cone", bbox, volume, area);
    shape->params = {radius1, radius2, height};
    shape->metadata["primitive"] = "cone";
#ifndef CAIDE_STUB_MODE
    shape->native_shape = BRepPrimAPI_MakeCone(radius1, radius2, height).Shape();
    populate_occ_metrics(*shape);
#endif
    clear_error();
    return {Status::ok(), finalize_shape(doc, shape, "create_cone", "{\"radius1\":" + format_double(radius1) + ",\"radius2\":" + format_double(radius2) + ",\"height\":" + format_double(height) + "}")};
}

Result<std::shared_ptr<ShapeData>> create_torus(DocumentData& doc, double major_radius, double minor_radius) {
    if (auto status = validate_positive("major_radius", major_radius); !status) {
        return {set_error(status.code, status.message), nullptr};
    }
    if (auto status = validate_positive("minor_radius", minor_radius); !status) {
        return {set_error(status.code, status.message), nullptr};
    }
    if (minor_radius >= major_radius) {
        return {set_error(CAIDE_ERR_INVALID_PARAM, "minor_radius must be smaller than major_radius"), nullptr};
    }

    const double outer = major_radius + minor_radius;
    const BoundingBox bbox = make_bbox({-outer, -outer, -minor_radius}, {outer, outer, minor_radius});
    auto shape = make_shape(ShapeKind::Torus, "torus", bbox, 2.0 * kPi * kPi * major_radius * minor_radius * minor_radius, 4.0 * kPi * kPi * major_radius * minor_radius);
    shape->params = {major_radius, minor_radius};
    shape->metadata["primitive"] = "torus";
#ifndef CAIDE_STUB_MODE
    shape->native_shape = BRepPrimAPI_MakeTorus(major_radius, minor_radius).Shape();
    populate_occ_metrics(*shape);
#endif
    clear_error();
    return {Status::ok(), finalize_shape(doc, shape, "create_torus", "{\"major_radius\":" + format_double(major_radius) + ",\"minor_radius\":" + format_double(minor_radius) + "}")};
}

}  // namespace caide::geometry
