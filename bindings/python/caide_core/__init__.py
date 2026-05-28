"""Pythonic high-level wrapper over libcaide_core.

Provides RAII-style handle wrappers that release native resources in
``__del__``. The :class:`Document` is the natural entry point::

    from caide_core import Document, ExportFormat

    with Document() as doc:
        cube = doc.cube(10, 10, 10)
        cube.export("cube.stl", ExportFormat.STL_BINARY)
"""

from __future__ import annotations

import enum
import os
from contextlib import AbstractContextManager
from ctypes import (
    POINTER,
    byref,
    c_char_p,
    c_double,
    c_int,
    c_size_t,
    c_uint8,
)
from typing import Optional

from . import _ffi


__version__ = "0.1.0"


# ---------------------------------------------------------------------------
# Errors
# ---------------------------------------------------------------------------


class CaideError(RuntimeError):
    """Raised when a libcaide_core call returns a non-OK error code."""

    def __init__(self, code: int, function: str, message: Optional[str] = None) -> None:
        self.code = code
        self.function = function
        kernel_msg = _ffi.caide_last_error_message()
        decoded = (
            kernel_msg.decode("utf-8", errors="replace") if kernel_msg else ""
        )
        name = _ffi.ERROR_NAMES.get(code, f"code={code}")
        text = message or decoded or "unknown error"
        super().__init__(f"{function} failed [{name}]: {text}")


def _check(code: int, function: str) -> None:
    if code != _ffi.CAIDE_OK:
        raise CaideError(code, function)


# ---------------------------------------------------------------------------
# Enums (Pythonic mirrors of C enums)
# ---------------------------------------------------------------------------


class ExportFormat(enum.IntEnum):
    STL_BINARY = _ffi.CAIDE_FORMAT_STL_BINARY
    STL_ASCII = _ffi.CAIDE_FORMAT_STL_ASCII
    OBJ = _ffi.CAIDE_FORMAT_OBJ
    THREE_MF = _ffi.CAIDE_FORMAT_3MF
    STEP = _ffi.CAIDE_FORMAT_STEP
    GLTF = _ffi.CAIDE_FORMAT_GLTF


class PrintSeverity(enum.IntEnum):
    OK = _ffi.CAIDE_PRINT_OK
    WARNING = _ffi.CAIDE_PRINT_WARNING
    CRITICAL = _ffi.CAIDE_PRINT_CRITICAL


# ---------------------------------------------------------------------------
# Version helpers
# ---------------------------------------------------------------------------


def version() -> str:
    """Return the libcaide_core semantic version string."""
    raw = _ffi.caide_version()
    return raw.decode("utf-8") if raw else ""


def version_tuple() -> tuple[int, int, int]:
    return (
        _ffi.caide_version_major(),
        _ffi.caide_version_minor(),
        _ffi.caide_version_patch(),
    )


# ---------------------------------------------------------------------------
# Handle wrappers
# ---------------------------------------------------------------------------


class Shape:
    """A handle to a CAD shape. Released on ``__del__``."""

    __slots__ = ("_handle", "_owned")

    def __init__(self, handle, *, owned: bool = True) -> None:
        self._handle = handle
        self._owned = owned

    @property
    def handle(self):
        if self._handle is None:
            raise RuntimeError("Shape handle has been closed")
        return self._handle

    def close(self) -> None:
        if self._owned and self._handle is not None:
            _ffi.caide_shape_destroy(self._handle)
        self._handle = None

    def __del__(self) -> None:  # pragma: no cover - GC timing
        try:
            self.close()
        except Exception:
            pass

    def __enter__(self) -> "Shape":
        return self

    def __exit__(self, *exc) -> None:
        self.close()

    # ----- queries -----

    def is_valid(self) -> bool:
        return bool(_ffi.caide_shape_is_valid(self.handle))

    def volume(self) -> float:
        out = c_double()
        _check(_ffi.caide_shape_volume(self.handle, byref(out)), "caide_shape_volume")
        return out.value

    def surface_area(self) -> float:
        out = c_double()
        _check(
            _ffi.caide_shape_surface_area(self.handle, byref(out)),
            "caide_shape_surface_area",
        )
        return out.value

    def bounding_box(self) -> tuple[float, float, float, float, float, float]:
        mins = [c_double(), c_double(), c_double()]
        maxs = [c_double(), c_double(), c_double()]
        _check(
            _ffi.caide_shape_bounding_box(
                self.handle,
                byref(mins[0]), byref(mins[1]), byref(mins[2]),
                byref(maxs[0]), byref(maxs[1]), byref(maxs[2]),
            ),
            "caide_shape_bounding_box",
        )
        return (
            mins[0].value, mins[1].value, mins[2].value,
            maxs[0].value, maxs[1].value, maxs[2].value,
        )

    def clone(self) -> "Shape":
        out = _ffi.CaideShape()
        _check(_ffi.caide_shape_clone(self.handle, byref(out)), "caide_shape_clone")
        return Shape(out)

    # ----- boolean operators -----

    def union(self, other: "Shape") -> "Shape":
        out = _ffi.CaideShape()
        _check(
            _ffi.caide_boolean_union(self.handle, other.handle, byref(out)),
            "caide_boolean_union",
        )
        return Shape(out)

    def subtract(self, other: "Shape") -> "Shape":
        out = _ffi.CaideShape()
        _check(
            _ffi.caide_boolean_subtract(self.handle, other.handle, byref(out)),
            "caide_boolean_subtract",
        )
        return Shape(out)

    def intersect(self, other: "Shape") -> "Shape":
        out = _ffi.CaideShape()
        _check(
            _ffi.caide_boolean_intersect(self.handle, other.handle, byref(out)),
            "caide_boolean_intersect",
        )
        return Shape(out)

    # ----- transforms -----

    def translate(self, dx: float, dy: float, dz: float) -> "Shape":
        out = _ffi.CaideShape()
        _check(
            _ffi.caide_transform_translate(self.handle, dx, dy, dz, byref(out)),
            "caide_transform_translate",
        )
        return Shape(out)

    def rotate(self, axis: tuple[float, float, float], angle_deg: float) -> "Shape":
        out = _ffi.CaideShape()
        ax, ay, az = axis
        _check(
            _ffi.caide_transform_rotate(self.handle, ax, ay, az, angle_deg, byref(out)),
            "caide_transform_rotate",
        )
        return Shape(out)

    def scale(self, center: tuple[float, float, float], factor: float) -> "Shape":
        out = _ffi.CaideShape()
        cx, cy, cz = center
        _check(
            _ffi.caide_transform_scale(self.handle, cx, cy, cz, factor, byref(out)),
            "caide_transform_scale",
        )
        return Shape(out)

    def mirror(self, normal: tuple[float, float, float], d: float = 0.0) -> "Shape":
        out = _ffi.CaideShape()
        nx, ny, nz = normal
        _check(
            _ffi.caide_transform_mirror(self.handle, nx, ny, nz, d, byref(out)),
            "caide_transform_mirror",
        )
        return Shape(out)

    # ----- features -----

    def fillet(self, radius: float, edges: Optional[list[int]] = None) -> "Shape":
        out = _ffi.CaideShape()
        arr, count = _int_array(edges)
        _check(
            _ffi.caide_feature_fillet(self.handle, radius, arr, count, byref(out)),
            "caide_feature_fillet",
        )
        return Shape(out)

    def chamfer(self, distance: float, edges: Optional[list[int]] = None) -> "Shape":
        out = _ffi.CaideShape()
        arr, count = _int_array(edges)
        _check(
            _ffi.caide_feature_chamfer(self.handle, distance, arr, count, byref(out)),
            "caide_feature_chamfer",
        )
        return Shape(out)

    # ----- export -----

    def export(
        self,
        path: str | os.PathLike,
        fmt: ExportFormat,
        *,
        linear_deflection: float = 0.1,
        angular_deflection: float = 0.5,
        relative: bool = False,
    ) -> None:
        params = _ffi.CaideTessellationParams(
            linear_deflection, angular_deflection, 1 if relative else 0
        )
        _check(
            _ffi.caide_export_to_file(
                self.handle,
                int(fmt),
                os.fspath(path).encode("utf-8"),
                byref(params),
            ),
            "caide_export_to_file",
        )

    def export_to_bytes(
        self,
        fmt: ExportFormat,
        *,
        linear_deflection: float = 0.1,
        angular_deflection: float = 0.5,
        relative: bool = False,
    ) -> bytes:
        params = _ffi.CaideTessellationParams(
            linear_deflection, angular_deflection, 1 if relative else 0
        )
        buf_ptr = POINTER(c_uint8)()
        size = c_size_t(0)
        _check(
            _ffi.caide_export_to_buffer(
                self.handle, int(fmt), byref(params), byref(buf_ptr), byref(size)
            ),
            "caide_export_to_buffer",
        )
        try:
            if not buf_ptr:
                return b""
            return bytes(bytearray(buf_ptr[i] for i in range(size.value)))
        finally:
            if buf_ptr:
                _ffi.caide_buffer_free(buf_ptr)


def _int_array(values: Optional[list[int]]):
    if not values:
        return (None, 0)
    arr_type = c_int * len(values)
    arr = arr_type(*values)
    return (arr, len(values))


class Document(AbstractContextManager):
    """A CAD document: owns a context and exposes primitive constructors."""

    def __init__(self) -> None:
        self._ctx = _ffi.CaideContext()
        _check(_ffi.caide_context_create(byref(self._ctx)), "caide_context_create")
        self._doc = _ffi.CaideDocument()
        try:
            _check(
                _ffi.caide_document_create(self._ctx, byref(self._doc)),
                "caide_document_create",
            )
        except CaideError:
            _ffi.caide_context_destroy(self._ctx)
            self._ctx = None
            raise

    @property
    def handle(self):
        if self._doc is None:
            raise RuntimeError("Document has been closed")
        return self._doc

    def close(self) -> None:
        if self._doc is not None:
            _ffi.caide_document_destroy(self._doc)
            self._doc = None
        if self._ctx is not None:
            _ffi.caide_context_destroy(self._ctx)
            self._ctx = None

    def __del__(self) -> None:  # pragma: no cover - GC timing
        try:
            self.close()
        except Exception:
            pass

    def __exit__(self, *exc) -> None:
        self.close()

    # ----- primitives -----

    def box(self, width: float, height: float, depth: float) -> Shape:
        out = _ffi.CaideShape()
        _check(
            _ffi.caide_shape_create_box(self.handle, width, height, depth, byref(out)),
            "caide_shape_create_box",
        )
        return Shape(out)

    # Alias: cube == box with equal sides
    def cube(self, size: float) -> Shape:
        return self.box(size, size, size)

    def cylinder(self, radius: float, height: float) -> Shape:
        out = _ffi.CaideShape()
        _check(
            _ffi.caide_shape_create_cylinder(self.handle, radius, height, byref(out)),
            "caide_shape_create_cylinder",
        )
        return Shape(out)

    def sphere(self, radius: float) -> Shape:
        out = _ffi.CaideShape()
        _check(
            _ffi.caide_shape_create_sphere(self.handle, radius, byref(out)),
            "caide_shape_create_sphere",
        )
        return Shape(out)

    def cone(self, radius1: float, radius2: float, height: float) -> Shape:
        out = _ffi.CaideShape()
        _check(
            _ffi.caide_shape_create_cone(self.handle, radius1, radius2, height, byref(out)),
            "caide_shape_create_cone",
        )
        return Shape(out)

    def torus(self, major_radius: float, minor_radius: float) -> Shape:
        out = _ffi.CaideShape()
        _check(
            _ffi.caide_shape_create_torus(self.handle, major_radius, minor_radius, byref(out)),
            "caide_shape_create_torus",
        )
        return Shape(out)

    # ----- history -----

    def undo(self) -> None:
        _check(_ffi.caide_document_undo(self.handle), "caide_document_undo")

    def redo(self) -> None:
        _check(_ffi.caide_document_redo(self.handle), "caide_document_redo")

    def history_count(self) -> int:
        return int(_ffi.caide_document_history_count(self.handle))


__all__ = [
    "CaideError",
    "Document",
    "ExportFormat",
    "PrintSeverity",
    "Shape",
    "__version__",
    "version",
    "version_tuple",
]
