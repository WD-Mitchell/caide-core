# caide-core

`caide-core` is the shared C++ CAD kernel for the Caide platform. It exposes a stable C ABI so Swift, Python, Rust, C#, and plugin runtimes can all talk to the same geometry engine.

## Goals

- **Cross-platform:** buildable on macOS, Linux, and Windows.
- **CAD kernel abstraction:** hide OpenCascade behind a compact, FFI-friendly C ABI.
- **Stub mode:** compile and exercise the library even when OpenCascade is not installed.
- **Agent-ready:** geometry commands, history, export, printability analysis, and stable references are all modeled as deterministic services.

## Architecture

### 1. Public C ABI

The public interface lives in `include/caide_core/caide_core.h` and exposes:

- semantic versioning helpers
- thread-local error handling
- opaque handles for context, document, shapes, sketches, meshes, and references
- primitive creation, booleans, transforms, features, sketching, and advanced operations
- export/tessellation APIs for renderer and slicer integration
- printability analysis APIs
- command execution and history APIs
- stable reference creation and lookup

### 2. Internal C++ Model

Internally the library keeps a lightweight geometry model that is always available:

- `ShapeData` stores semantic shape type, metrics, sampled points, transform state, and metadata.
- `DocumentData` owns parametric history and reference resolution.
- `SketchData` captures 2D curve entities and converts them into wires/faces.
- `MeshData` is a renderer-friendly triangle mesh with float vertices and normals.
- `ReferenceData` provides stable IDs across command execution and undo/redo.

When OpenCascade is available, the same shape objects also carry native `TopoDS_Shape` state.

### 3. Stub Mode

If `find_package(OpenCASCADE)` fails, the build switches to `CAIDE_STUB_MODE` automatically.
Stub mode still provides:

- physically plausible bounding boxes, volume, and surface area estimates
- boolean operations using envelope heuristics
- transform propagation via explicit matrices
- mesh generation for primitives and generic solids
- command execution, history, export, references, and printability analysis
- a full C ABI test surface for downstream integrations

That makes this repository usable in CI, on contributor machines, and inside language bindings before native OCCT provisioning is in place.

## Build

### CMake

```bash
cmake -S . -B build -DCAIDE_BUILD_TESTS=ON -DCAIDE_BUILD_SHARED=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

### Conan

```bash
conan install . --output-folder=build --build=missing
cmake -S . -B build
cmake --build build
```

Set `-c user.caide:with_opencascade=True` if your Conan profile provides OpenCascade.

## Export Support

The export layer supports these formats through a single API:

- STL (binary + ASCII)
- OBJ
- 3MF
- STEP
- glTF placeholder payload for renderer integration

In stub mode the non-mesh formats are intentionally lightweight text payloads that preserve shape metadata and keep the ABI stable. When OCCT is wired in on a target platform, the same surface is ready to be upgraded with native exporters.

## Printability Analysis

The printability subsystem ships with a deterministic heuristic pass intended for fast feedback loops:

- minimum wall thickness estimation
- overhang risk inference
- bridge span warnings
- minimum feature size checks
- support volume estimation
- bed adhesion area calculation

That makes it suitable for agentic repair loops where the frontend or backend wants immediate, explainable feedback before handing work to a full slicer.

## Tests

The repository includes:

- C++ unit tests for primitives, booleans, transforms, features, sketches, advanced operations, export, printability, command execution, and references
- a pure C ABI test that only includes the public header and links against the built library
- a GitHub Actions matrix that compiles and runs tests on macOS, Linux, and Windows

## Embedding from Other Languages

The C ABI is designed for straightforward FFI:

- Swift can import the header through a modulemap or package target.
- Python can bind through `ctypes` or CFFI.
- Rust can use `bindgen` or handwritten extern declarations.
- .NET can bind through P/Invoke.

Opaque handles keep ownership rules explicit and prevent C++ type leakage across the ABI boundary.

## Roadmap Alignment

This repository is intentionally scoped to support the wider Caide platform goals:

- shared CAD kernel for macOS, iPadOS, backend services, and printer-side tools
- deterministic command execution for agent-generated CAD workflows
- printability-first feedback loops
- stable references for parametric history replay
- transport-friendly buffers and tessellation for real-time UI integration

## Example: C API Session

```c
#include <stdio.h>
#include <stdlib.h>
#include "caide_core/caide_core.h"

int main(void) {
    CaideContext ctx = NULL;
    CaideDocument doc = NULL;
    CaideShape box = NULL;
    CaideShape moved = NULL;
    CaideMesh mesh = NULL;
    uint8_t* obj = NULL;
    size_t obj_size = 0;

    if (caide_context_create(&ctx) != CAIDE_OK) {
        fprintf(stderr, "context error: %s\n", caide_last_error_message());
        return 1;
    }

    if (caide_document_create(ctx, &doc) != CAIDE_OK) {
        fprintf(stderr, "document error: %s\n", caide_last_error_message());
        caide_context_destroy(ctx);
        return 1;
    }

    if (caide_shape_create_box(doc, 120.0, 80.0, 30.0, &box) != CAIDE_OK) {
        fprintf(stderr, "box error: %s\n", caide_last_error_message());
        caide_document_destroy(doc);
        caide_context_destroy(ctx);
        return 1;
    }

    if (caide_transform_translate(box, 0.0, 0.0, 12.0, &moved) != CAIDE_OK) {
        fprintf(stderr, "translate error: %s\n", caide_last_error_message());
        caide_shape_destroy(box);
        caide_document_destroy(doc);
        caide_context_destroy(ctx);
        return 1;
    }

    if (caide_mesh_tessellate(moved, NULL, &mesh) != CAIDE_OK) {
        fprintf(stderr, "mesh error: %s\n", caide_last_error_message());
        caide_shape_destroy(moved);
        caide_shape_destroy(box);
        caide_document_destroy(doc);
        caide_context_destroy(ctx);
        return 1;
    }

    if (caide_export_to_buffer(moved, CAIDE_FORMAT_OBJ, NULL, &obj, &obj_size) != CAIDE_OK) {
        fprintf(stderr, "export error: %s\n", caide_last_error_message());
        caide_mesh_destroy(mesh);
        caide_shape_destroy(moved);
        caide_shape_destroy(box);
        caide_document_destroy(doc);
        caide_context_destroy(ctx);
        return 1;
    }

    printf("OBJ bytes: %zu\n", obj_size);

    caide_buffer_free(obj);
    caide_mesh_destroy(mesh);
    caide_shape_destroy(moved);
    caide_shape_destroy(box);
    caide_document_destroy(doc);
    caide_context_destroy(ctx);
    return 0;
}
```

## Example: Command Executor JSON

The command executor is intentionally flat and deterministic. Example payloads:

```json
{"type":"create_box","params_json":"{\"width\":100,\"height\":60,\"depth\":20}"}
{"type":"translate","target_ref":"shape-1","params_json":"{\"dx\":0,\"dy\":0,\"dz\":10}"}
{"type":"fillet","target_ref":"cmd-3","params_json":"{\"radius\":1.5,\"edge_indices\":[0,1,2,3]}"}
{"type":"boolean_union","target_ref":"cmd-3","params_json":"{\"ref_b\":\"shape-2\"}"}
{"type":"extrude","target_ref":"face-1","params_json":"{\"dx\":0,\"dy\":0,\"dz\":25}"}
{"type":"revolve","target_ref":"face-2","params_json":"{\"axis_x\":0,\"axis_y\":1,\"axis_z\":0,\"angle_deg\":270}"}
{"type":"sweep","target_ref":"profile-1","params_json":"{\"spine_ref\":\"wire-2\"}"}
{"type":"loft","params_json":"{\"profile_refs\":[\"wire-3\",\"wire-4\",\"wire-5\"]}"}
```

## Example: Swift Bridging Notes

```swift
import Foundation

final class KernelSession {
    private var context: CaideContext?
    private var document: CaideDocument?

    init() throws {
        guard caide_context_create(&context) == CAIDE_OK else {
            throw NSError(domain: "caide", code: Int(caide_last_error_code().rawValue), userInfo: [NSLocalizedDescriptionKey: String(cString: caide_last_error_message())])
        }
        guard caide_document_create(context, &document) == CAIDE_OK else {
            throw NSError(domain: "caide", code: Int(caide_last_error_code().rawValue), userInfo: [NSLocalizedDescriptionKey: String(cString: caide_last_error_message())])
        }
    }

    deinit {
        if let document {
            caide_document_destroy(document)
        }
        if let context {
            caide_context_destroy(context)
        }
    }

    func makeBox() throws {
        var shape: CaideShape?
        guard caide_shape_create_box(document, 80, 40, 10, &shape) == CAIDE_OK else {
            throw NSError(domain: "caide", code: Int(caide_last_error_code().rawValue), userInfo: [NSLocalizedDescriptionKey: String(cString: caide_last_error_message())])
        }
        if let shape {
            caide_shape_destroy(shape)
        }
    }
}
```

## Module Reference

### `src/geometry/primitives.cpp`

- creates boxes, cylinders, spheres, cones, and tori
- computes bounding boxes, volumes, and surface areas in both stub and OCCT-backed modes
- registers the result into the owning document with stable reference IDs
- appends parametric history entries for replay and undo/redo

### `src/geometry/booleans.cpp`

- performs union, subtract, and intersect operations
- uses envelope heuristics in stub mode
- preserves child dependencies for later introspection, export, and command history
- reports topology overlap failures explicitly

### `src/geometry/transforms.cpp`

- applies translation, rotation, scale, and mirror operations
- stores explicit transform matrices on the resulting shape
- updates bounding boxes and derived metrics
- keeps shape lineage available for command graphs

### `src/geometry/features.cpp`

- exposes fillet, chamfer, shell, and draft operations
- validates requested radii, thicknesses, and draft direction vectors
- records selected edge and face indices in result metadata
- is structured to use OCCT feature builders when available

### `src/geometry/sketch.cpp`

- stores lines, arcs, circles, and splines as deterministic sketch entities
- converts those entities into sampled wire or face geometry
- computes planar area with polygon or analytic-circle paths
- supports downstream extrusion, revolve, sweep, and loft workflows

### `src/geometry/advanced.cpp`

- handles extrude, revolve, sweep, and loft
- estimates volume and surface area from profile area and path length
- builds predictable envelopes in stub mode for export and printability analysis
- is prepared to switch over to native OCCT sweep/revolve/loft builders

### `src/commands/executor.cpp`

- validates supported command types
- parses flat JSON without introducing a third-party dependency
- resolves references from document history
- executes high-level geometry workflows and records resulting history entries

### `src/export/mesh_export.cpp` and `src/export/tessellation.cpp`

- generate triangle meshes for renderers, previews, and export payloads
- synthesize STL, OBJ, 3MF, STEP placeholder, and glTF placeholder outputs
- keep the ABI stable regardless of whether OCCT is present
- expose mesh pointers for zero-copy renderer integration while the mesh handle is alive

### `src/printability/analyzer.cpp`

- computes wall thickness warnings, inferred overhang risk, bridge checks, and support estimates
- returns explainable issue reasons so agentic repair loops can react deterministically
- mirrors the future backend printability pipeline in a local synchronous form
- keeps issue allocation ABI-safe for pure C callers

### `src/references/naming.cpp`

- generates stable IDs independent of raw pointer addresses
- resolves references through document-owned maps
- tracks invalidation for undo/redo and future topological naming upgrades
- provides the baseline handle identity layer for higher-level parametric workflows

## Operational Notes

- Shape reference IDs are generated inside the document and preserved through history entries.
- Export buffers returned by `caide_export_to_buffer` must be released with `caide_buffer_free`.
- Mesh vertex/normal/index pointers returned by the mesh accessors remain valid only while the `CaideMesh` handle is alive.
- Printability issue arrays must be released with `caide_print_issues_free`.
- The library uses thread-local error state, so callers should read error messages on the same thread that made the failing API call.
- Stub mode is designed to be deterministic; it is not a placeholder that simply returns `CAIDE_ERR_NOT_IMPLEMENTED`.
- CI intentionally exercises stub mode first so contributors can validate the ABI without an OCCT toolchain.
- The same shape metadata is structured to feed backend repair loops and UI overlays.
- Command history entries are plain data and can be serialized by a higher-level host application.
- The repository is ready to become the shared kernel dependency for Caide desktop, tablet, backend, and printer flows.
- All public entry points are ABI-stable C functions behind opaque handles.
- All internal modules are organized to make future OCCT specialization incremental rather than disruptive.
