# caide_core — Python bindings

Pure-Python ctypes bindings for `libcaide_core`, the caide CAD kernel.

## Requirements

- Python 3.9+
- A built copy of `libcaide_core.{so,dylib,dll}` from the parent C++ project.

Build the kernel from the repo root:

```sh
cmake -S . -B build
cmake --build build --target caide_core
```

## Install

From this directory:

```sh
pip install -e .
# or with tests
pip install -e ".[dev]"
```

## Library discovery

The package looks for the shared library in this order:

1. `CAIDE_CORE_LIB` environment variable (absolute path to the library).
2. Common build directories alongside the repo root: `build/`, `build_check/`,
   `build_occt/`, `build_stub/`, etc. (and any sibling `build*/src/` directory).
3. The system loader (`ctypes.util.find_library("caide_core")`).

If you have a custom build location, point at the file explicitly:

```sh
export CAIDE_CORE_LIB=/abs/path/to/libcaide_core.dylib
```

## Quick start

```python
from caide_core import Document, ExportFormat, version

print("kernel:", version())

with Document() as doc:
    cube = doc.cube(20.0)                # 20mm cube
    cyl  = doc.cylinder(radius=6, height=30)

    carved = cube.subtract(cyl)
    carved.export("cube_with_hole.stl", ExportFormat.STL_BINARY)

    # other primitives + features
    sph = doc.sphere(10)
    moved = sph.translate(5, 0, 0)
    bbox = moved.bounding_box()
    print("bbox:", bbox, "volume:", moved.volume())
```

All `Shape` and `Document` objects release their native handles automatically
when garbage-collected, and also support explicit `close()` / `with` blocks
for deterministic cleanup.

## Errors

Any non-zero error code from the C API raises a `caide_core.CaideError`
including the kernel's `caide_last_error_message()` text:

```python
from caide_core import CaideError, Document

with Document() as doc:
    try:
        doc.box(-1, 1, 1)                # invalid dimensions
    except CaideError as exc:
        print("kernel rejected:", exc.code, exc)
```

## Running the smoke tests

```sh
cd bindings/python
CAIDE_CORE_LIB=/abs/path/to/libcaide_core.dylib pytest -v
```

If the library is already in one of the discovered build directories the env
var isn't needed.
