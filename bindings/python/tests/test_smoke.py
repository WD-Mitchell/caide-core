"""Smoke tests for the caide_core Python bindings."""

from __future__ import annotations

import os
from pathlib import Path

import pytest

import caide_core
from caide_core import Document, ExportFormat


def test_version() -> None:
    v = caide_core.version()
    assert isinstance(v, str)
    assert v, "version string must be non-empty"
    major, minor, patch = caide_core.version_tuple()
    assert all(isinstance(x, int) for x in (major, minor, patch))
    assert major >= 0


def test_create_cube() -> None:
    with Document() as doc:
        shape = doc.cube(10.0)
        try:
            assert shape.is_valid()
            volume = shape.volume()
            assert volume > 0.0
            bbox = shape.bounding_box()
            assert len(bbox) == 6
        finally:
            shape.close()


def test_export_stl(tmp_path: Path) -> None:
    out_path = tmp_path / "cube.stl"
    with Document() as doc:
        with doc.cube(20.0) as shape:
            shape.export(out_path, ExportFormat.STL_BINARY)
    assert out_path.exists(), "STL export should create the file"
    assert out_path.stat().st_size > 0, "STL file should not be empty"


def test_export_to_bytes_roundtrip() -> None:
    with Document() as doc:
        with doc.cube(5.0) as shape:
            data = shape.export_to_bytes(ExportFormat.OBJ)
    assert isinstance(data, (bytes, bytearray))
    assert len(data) > 0


def test_boolean_union_and_subtract() -> None:
    with Document() as doc:
        a = doc.box(10, 10, 10)
        b = doc.sphere(6)
        try:
            with a.union(b) as combined:
                assert combined.is_valid()
            with a.subtract(b) as carved:
                assert carved.is_valid()
        finally:
            a.close()
            b.close()


def test_transform_translate_rotate() -> None:
    with Document() as doc:
        with doc.cylinder(3.0, 8.0) as cyl:
            with cyl.translate(1, 2, 3) as moved:
                assert moved.is_valid()
            with cyl.rotate((0, 0, 1), 45.0) as rotated:
                assert rotated.is_valid()


def test_sketch_to_face_roundtrip() -> None:
    """Build a closed 10x10 square sketch -> face. Surface area should be 100."""
    with Document() as doc:
        with doc.sketch() as sk:
            (
                sk.add_line(0.0, 0.0, 10.0, 0.0)
                  .add_line(10.0, 0.0, 10.0, 10.0)
                  .add_line(10.0, 10.0, 0.0, 10.0)
                  .add_line(0.0, 10.0, 0.0, 0.0)
                  .close_profile()
            )
            with sk.to_face() as face:
                assert face.is_valid()
                assert abs(face.surface_area() - 100.0) < 1e-6
            with sk.to_wire() as wire:
                assert wire.is_valid()


def test_sketch_circle_and_spline() -> None:
    with Document() as doc:
        with doc.sketch() as circle:
            circle.add_circle(0.0, 0.0, 5.0)
            with circle.to_face() as face:
                assert face.surface_area() > 75.0  # ~78.5

        with doc.sketch() as curve:
            curve.add_spline([(0, 0), (2, 3), (4, 3), (6, 0)])
            curve.add_arc(3.0, 0.0, 3.0, 0.0, 180.0)
            with curve.to_wire() as wire:
                assert wire.is_valid()


def test_shape_extrude_and_revolve() -> None:
    with Document() as doc:
        # Square face -> extrude into a 10x10x20 prism
        with doc.sketch() as sk:
            (sk.add_line(0, 0, 10, 0)
               .add_line(10, 0, 10, 10)
               .add_line(10, 10, 0, 10)
               .add_line(0, 10, 0, 0)
               .close_profile())
            with sk.to_face() as face:
                with face.extrude(0.0, 0.0, 20.0) as prism:
                    assert prism.is_valid()
                    assert prism.volume() > 0.0
                with face.revolve(axis=(0.0, 1.0, 0.0), angle_deg=180.0) as rev:
                    assert rev.is_valid()
                    assert rev.volume() > 0.0


def test_shape_sweep_and_loft() -> None:
    with Document() as doc:
        # profile = square face
        with doc.sketch() as sk:
            (sk.add_line(0, 0, 2, 0)
               .add_line(2, 0, 2, 2)
               .add_line(2, 2, 0, 2)
               .add_line(0, 2, 0, 0)
               .close_profile())
            face = sk.to_face()

        # spine = a wire
        with doc.sketch() as path:
            path.add_line(0, 0, 0, 5)
            spine = path.to_wire()

        with face.sweep(spine) as swept:
            assert swept.is_valid()

        # loft requires >= 2 profiles
        with doc.sketch() as sk2:
            sk2.add_circle(0.0, 0.0, 3.0)
            face2 = sk2.to_face()
        with doc.sketch() as sk3:
            sk3.add_circle(0.0, 0.0, 1.5)
            face3 = sk3.to_face()
        with doc.loft([face2, face3]) as lofted:
            assert lofted.is_valid()
            assert lofted.volume() > 0.0

        face.close()
        spine.close()
        face2.close()
        face3.close()


def test_printability_metrics_on_cube() -> None:
    with Document() as doc:
        with doc.cube(20.0) as cube:
            thickness = cube.wall_thickness()
            assert thickness > 0.0
            adhesion = cube.bed_adhesion_area()
            # A 20mm cube sitting Z-up should have a non-zero bottom face area.
            assert adhesion > 0.0
            sup = cube.support_volume()
            # A simple cube should require no support material.
            assert sup >= 0.0


def test_shape_shell_and_draft() -> None:
    with Document() as doc:
        with doc.cube(10.0) as cube:
            with cube.shell(thickness=1.0) as shelled:
                assert shelled.is_valid()
            with cube.draft(angle_deg=5.0, direction=(0, 0, 1)) as drafted:
                assert drafted.is_valid()
