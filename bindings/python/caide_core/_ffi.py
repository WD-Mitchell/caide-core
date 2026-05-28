"""Low-level ctypes bindings for libcaide_core.

Mirrors the C ABI declared in ``include/caide_core/caide_core.h``.

The shared library is discovered in the following order:

1. The path given by the ``CAIDE_CORE_LIB`` environment variable.
2. Common build directories relative to the repository root
   (``build``, ``build_check``, ``build_occt``, ``build_stub``, ``build_verify``,
   ``build_req``, ``build_audit``, ``build_occt_cfg``).
3. The system library loader (``libcaide_core`` via :func:`ctypes.util.find_library`).
"""

from __future__ import annotations

import ctypes
import os
import platform
import sys
from ctypes import (
    CFUNCTYPE,
    POINTER,
    Structure,
    c_char_p,
    c_double,
    c_float,
    c_int,
    c_size_t,
    c_uint8,
    c_uint32,
    c_void_p,
)
from ctypes.util import find_library
from pathlib import Path
from typing import Iterable, List, Optional


# ---------------------------------------------------------------------------
# Library discovery
# ---------------------------------------------------------------------------


def _candidate_lib_names() -> List[str]:
    system = platform.system()
    if system == "Darwin":
        return ["libcaide_core.dylib"]
    if system == "Windows":
        return ["caide_core.dll", "libcaide_core.dll"]
    return ["libcaide_core.so"]


def _repo_root() -> Path:
    # bindings/python/caide_core/_ffi.py -> repo root is 3 levels up
    return Path(__file__).resolve().parents[3]


def _candidate_dirs() -> List[Path]:
    root = _repo_root()
    common = [
        "build",
        "build_check",
        "build_occt",
        "build_stub",
        "build_verify",
        "build_req",
        "build_audit",
        "build_occt_cfg",
    ]
    dirs: List[Path] = []
    for name in common:
        base = root / name
        dirs.append(base / "src")
        dirs.append(base)
        dirs.append(base / "lib")
    # Also walk any directories named build*/ for src/libcaide_core.*
    if root.exists():
        for entry in root.iterdir():
            if entry.is_dir() and entry.name.startswith("build") and entry / "src" not in dirs:
                dirs.append(entry / "src")
                dirs.append(entry)
    return dirs


def _discover_library_path() -> Optional[str]:
    explicit = os.environ.get("CAIDE_CORE_LIB")
    if explicit:
        if not os.path.isfile(explicit):
            raise FileNotFoundError(
                f"CAIDE_CORE_LIB={explicit!r} does not point to an existing file"
            )
        return explicit

    names = _candidate_lib_names()
    for directory in _candidate_dirs():
        for name in names:
            candidate = directory / name
            if candidate.is_file():
                return str(candidate)

    found = find_library("caide_core")
    return found


def _load_library() -> ctypes.CDLL:
    path = _discover_library_path()
    if path is None:
        raise OSError(
            "Could not locate libcaide_core. Build the kernel "
            "(e.g. `cmake --build build --target caide_core`) "
            "or set the CAIDE_CORE_LIB environment variable to the shared library path."
        )
    try:
        return ctypes.CDLL(path)
    except OSError as exc:  # pragma: no cover - depends on system loader
        raise OSError(f"Failed to load libcaide_core from {path!r}: {exc}") from exc


lib = _load_library()


# ---------------------------------------------------------------------------
# Opaque handles
# ---------------------------------------------------------------------------

CaideContext = c_void_p
CaideDocument = c_void_p
CaideShape = c_void_p
CaideSketch = c_void_p
CaideMesh = c_void_p
CaideReference = c_void_p


# ---------------------------------------------------------------------------
# Error codes (mirrors CaideError enum)
# ---------------------------------------------------------------------------

CAIDE_OK = 0
CAIDE_ERR_NULL_HANDLE = -1
CAIDE_ERR_INVALID_PARAM = -2
CAIDE_ERR_OPERATION_FAILED = -3
CAIDE_ERR_TOPOLOGY_ERROR = -4
CAIDE_ERR_REFERENCE_NOT_FOUND = -5
CAIDE_ERR_EXPORT_FAILED = -6
CAIDE_ERR_COMMAND_INVALID = -7
CAIDE_ERR_HISTORY_EMPTY = -8
CAIDE_ERR_OUT_OF_MEMORY = -9
CAIDE_ERR_NOT_IMPLEMENTED = -10
CAIDE_ERR_THREAD_SAFETY = -11

ERROR_NAMES = {
    CAIDE_OK: "OK",
    CAIDE_ERR_NULL_HANDLE: "NULL_HANDLE",
    CAIDE_ERR_INVALID_PARAM: "INVALID_PARAM",
    CAIDE_ERR_OPERATION_FAILED: "OPERATION_FAILED",
    CAIDE_ERR_TOPOLOGY_ERROR: "TOPOLOGY_ERROR",
    CAIDE_ERR_REFERENCE_NOT_FOUND: "REFERENCE_NOT_FOUND",
    CAIDE_ERR_EXPORT_FAILED: "EXPORT_FAILED",
    CAIDE_ERR_COMMAND_INVALID: "COMMAND_INVALID",
    CAIDE_ERR_HISTORY_EMPTY: "HISTORY_EMPTY",
    CAIDE_ERR_OUT_OF_MEMORY: "OUT_OF_MEMORY",
    CAIDE_ERR_NOT_IMPLEMENTED: "NOT_IMPLEMENTED",
    CAIDE_ERR_THREAD_SAFETY: "THREAD_SAFETY",
}


# Export format enum
CAIDE_FORMAT_STL_BINARY = 0
CAIDE_FORMAT_STL_ASCII = 1
CAIDE_FORMAT_OBJ = 2
CAIDE_FORMAT_3MF = 3
CAIDE_FORMAT_STEP = 4
CAIDE_FORMAT_GLTF = 5


# Printability severity
CAIDE_PRINT_OK = 0
CAIDE_PRINT_WARNING = 1
CAIDE_PRINT_CRITICAL = 2


# ---------------------------------------------------------------------------
# Structures
# ---------------------------------------------------------------------------


class CaideTessellationParams(Structure):
    _fields_ = [
        ("linear_deflection", c_double),
        ("angular_deflection", c_double),
        ("relative", c_int),
    ]


class CaidePrintIssue(Structure):
    _fields_ = [
        ("face_index", c_int),
        ("severity", c_int),
        ("reason", c_char_p),
        ("value", c_double),
    ]


class CaidePrintAnalysisParams(Structure):
    _fields_ = [
        ("overhang_threshold_deg", c_double),
        ("min_wall_thickness_mm", c_double),
        ("min_feature_size_mm", c_double),
        ("bridge_max_length_mm", c_double),
        ("build_dir_x", c_double),
        ("build_dir_y", c_double),
        ("build_dir_z", c_double),
    ]


class CaideCommand(Structure):
    _fields_ = [
        ("type", c_char_p),
        ("params_json", c_char_p),
        ("target_ref", c_char_p),
    ]


# ---------------------------------------------------------------------------
# Prototype binding helper
# ---------------------------------------------------------------------------


def _bind(name: str, restype, argtypes: Iterable):
    fn = getattr(lib, name)
    fn.restype = restype
    fn.argtypes = list(argtypes)
    return fn


# Version / error -----------------------------------------------------------

caide_version = _bind("caide_version", c_char_p, [])
caide_version_major = _bind("caide_version_major", c_int, [])
caide_version_minor = _bind("caide_version_minor", c_int, [])
caide_version_patch = _bind("caide_version_patch", c_int, [])

caide_last_error_message = _bind("caide_last_error_message", c_char_p, [])
caide_last_error_code = _bind("caide_last_error_code", c_int, [])

# Context -------------------------------------------------------------------

caide_context_create = _bind("caide_context_create", c_int, [POINTER(CaideContext)])
caide_context_destroy = _bind("caide_context_destroy", None, [CaideContext])

# Document ------------------------------------------------------------------

caide_document_create = _bind(
    "caide_document_create", c_int, [CaideContext, POINTER(CaideDocument)]
)
caide_document_destroy = _bind("caide_document_destroy", None, [CaideDocument])
caide_document_undo = _bind("caide_document_undo", c_int, [CaideDocument])
caide_document_redo = _bind("caide_document_redo", c_int, [CaideDocument])
caide_document_history_count = _bind("caide_document_history_count", c_int, [CaideDocument])
caide_document_get_shape = _bind(
    "caide_document_get_shape",
    c_int,
    [CaideDocument, c_char_p, POINTER(CaideShape)],
)

# Primitives ----------------------------------------------------------------

caide_shape_create_box = _bind(
    "caide_shape_create_box",
    c_int,
    [CaideDocument, c_double, c_double, c_double, POINTER(CaideShape)],
)
caide_shape_create_cylinder = _bind(
    "caide_shape_create_cylinder",
    c_int,
    [CaideDocument, c_double, c_double, POINTER(CaideShape)],
)
caide_shape_create_sphere = _bind(
    "caide_shape_create_sphere", c_int, [CaideDocument, c_double, POINTER(CaideShape)]
)
caide_shape_create_cone = _bind(
    "caide_shape_create_cone",
    c_int,
    [CaideDocument, c_double, c_double, c_double, POINTER(CaideShape)],
)
caide_shape_create_torus = _bind(
    "caide_shape_create_torus",
    c_int,
    [CaideDocument, c_double, c_double, POINTER(CaideShape)],
)

caide_shape_destroy = _bind("caide_shape_destroy", None, [CaideShape])
caide_shape_clone = _bind("caide_shape_clone", c_int, [CaideShape, POINTER(CaideShape)])
caide_shape_is_valid = _bind("caide_shape_is_valid", c_int, [CaideShape])

caide_shape_bounding_box = _bind(
    "caide_shape_bounding_box",
    c_int,
    [
        CaideShape,
        POINTER(c_double), POINTER(c_double), POINTER(c_double),
        POINTER(c_double), POINTER(c_double), POINTER(c_double),
    ],
)
caide_shape_volume = _bind("caide_shape_volume", c_int, [CaideShape, POINTER(c_double)])
caide_shape_surface_area = _bind(
    "caide_shape_surface_area", c_int, [CaideShape, POINTER(c_double)]
)

# Boolean ops ---------------------------------------------------------------

caide_boolean_union = _bind(
    "caide_boolean_union", c_int, [CaideShape, CaideShape, POINTER(CaideShape)]
)
caide_boolean_subtract = _bind(
    "caide_boolean_subtract", c_int, [CaideShape, CaideShape, POINTER(CaideShape)]
)
caide_boolean_intersect = _bind(
    "caide_boolean_intersect", c_int, [CaideShape, CaideShape, POINTER(CaideShape)]
)

# Transforms ----------------------------------------------------------------

caide_transform_translate = _bind(
    "caide_transform_translate",
    c_int,
    [CaideShape, c_double, c_double, c_double, POINTER(CaideShape)],
)
caide_transform_rotate = _bind(
    "caide_transform_rotate",
    c_int,
    [CaideShape, c_double, c_double, c_double, c_double, POINTER(CaideShape)],
)
caide_transform_scale = _bind(
    "caide_transform_scale",
    c_int,
    [CaideShape, c_double, c_double, c_double, c_double, POINTER(CaideShape)],
)
caide_transform_mirror = _bind(
    "caide_transform_mirror",
    c_int,
    [CaideShape, c_double, c_double, c_double, c_double, POINTER(CaideShape)],
)

# Features ------------------------------------------------------------------

caide_feature_fillet = _bind(
    "caide_feature_fillet",
    c_int,
    [CaideShape, c_double, POINTER(c_int), c_int, POINTER(CaideShape)],
)
caide_feature_chamfer = _bind(
    "caide_feature_chamfer",
    c_int,
    [CaideShape, c_double, POINTER(c_int), c_int, POINTER(CaideShape)],
)
caide_feature_shell = _bind(
    "caide_feature_shell",
    c_int,
    [CaideShape, c_double, POINTER(c_int), c_int, POINTER(CaideShape)],
)
caide_feature_draft = _bind(
    "caide_feature_draft",
    c_int,
    [
        CaideShape,
        c_double,
        c_double, c_double, c_double,
        POINTER(c_int), c_int,
        POINTER(CaideShape),
    ],
)

# Export --------------------------------------------------------------------

caide_export_to_file = _bind(
    "caide_export_to_file",
    c_int,
    [CaideShape, c_int, c_char_p, POINTER(CaideTessellationParams)],
)
caide_export_to_buffer = _bind(
    "caide_export_to_buffer",
    c_int,
    [
        CaideShape, c_int,
        POINTER(CaideTessellationParams),
        POINTER(POINTER(c_uint8)),
        POINTER(c_size_t),
    ],
)
caide_buffer_free = _bind("caide_buffer_free", None, [POINTER(c_uint8)])

caide_mesh_tessellate = _bind(
    "caide_mesh_tessellate",
    c_int,
    [CaideShape, POINTER(CaideTessellationParams), POINTER(CaideMesh)],
)
caide_mesh_destroy = _bind("caide_mesh_destroy", None, [CaideMesh])
caide_mesh_vertex_count = _bind("caide_mesh_vertex_count", c_int, [CaideMesh])
caide_mesh_triangle_count = _bind("caide_mesh_triangle_count", c_int, [CaideMesh])
caide_mesh_get_vertices = _bind(
    "caide_mesh_get_vertices",
    c_int,
    [CaideMesh, POINTER(POINTER(c_float)), POINTER(c_int)],
)
caide_mesh_get_normals = _bind(
    "caide_mesh_get_normals",
    c_int,
    [CaideMesh, POINTER(POINTER(c_float)), POINTER(c_int)],
)
caide_mesh_get_indices = _bind(
    "caide_mesh_get_indices",
    c_int,
    [CaideMesh, POINTER(POINTER(c_uint32)), POINTER(c_int)],
)

# Printability --------------------------------------------------------------

caide_print_analyze = _bind(
    "caide_print_analyze",
    c_int,
    [
        CaideShape,
        POINTER(CaidePrintAnalysisParams),
        POINTER(POINTER(CaidePrintIssue)),
        POINTER(c_int),
    ],
)
caide_print_issues_free = _bind(
    "caide_print_issues_free", None, [POINTER(CaidePrintIssue), c_int]
)

# Commands ------------------------------------------------------------------

caide_command_execute = _bind(
    "caide_command_execute",
    c_int,
    [CaideDocument, POINTER(CaideCommand), POINTER(CaideShape)],
)
caide_command_execute_batch = _bind(
    "caide_command_execute_batch",
    c_int,
    [CaideDocument, POINTER(CaideCommand), c_int],
)
caide_command_validate = _bind("caide_command_validate", c_int, [POINTER(CaideCommand)])

# References ----------------------------------------------------------------

caide_reference_create = _bind(
    "caide_reference_create",
    c_int,
    [CaideShape, c_char_p, POINTER(CaideReference)],
)
caide_reference_destroy = _bind("caide_reference_destroy", None, [CaideReference])
caide_reference_resolve = _bind(
    "caide_reference_resolve",
    c_int,
    [CaideDocument, c_char_p, POINTER(CaideShape)],
)
caide_reference_get_id = _bind("caide_reference_get_id", c_char_p, [CaideReference])
caide_reference_is_valid = _bind("caide_reference_is_valid", c_int, [CaideReference])


__all__ = [name for name in globals() if name.startswith("caide_") or name.startswith("CAIDE_") or name.startswith("Caide")]
