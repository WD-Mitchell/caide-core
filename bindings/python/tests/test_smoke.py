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
