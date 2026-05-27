#ifndef CAIDE_CORE_H
#define CAIDE_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/* ============================================================
 * Version & Initialization
 * ============================================================ */

#ifdef _WIN32
    #ifdef CAIDE_CORE_EXPORTS
        #define CAIDE_API __declspec(dllexport)
    #else
        #define CAIDE_API __declspec(dllimport)
    #endif
#else
    #define CAIDE_API __attribute__((visibility("default")))
#endif

/* Semantic version */
CAIDE_API const char* caide_version(void);
CAIDE_API int caide_version_major(void);
CAIDE_API int caide_version_minor(void);
CAIDE_API int caide_version_patch(void);

/* ============================================================
 * Error Handling
 * ============================================================ */

typedef enum {
    CAIDE_OK = 0,
    CAIDE_ERR_NULL_HANDLE = -1,
    CAIDE_ERR_INVALID_PARAM = -2,
    CAIDE_ERR_OPERATION_FAILED = -3,
    CAIDE_ERR_TOPOLOGY_ERROR = -4,
    CAIDE_ERR_REFERENCE_NOT_FOUND = -5,
    CAIDE_ERR_EXPORT_FAILED = -6,
    CAIDE_ERR_COMMAND_INVALID = -7,
    CAIDE_ERR_HISTORY_EMPTY = -8,
    CAIDE_ERR_OUT_OF_MEMORY = -9,
    CAIDE_ERR_NOT_IMPLEMENTED = -10,
    CAIDE_ERR_THREAD_SAFETY = -11,
} CaideError;

/* Thread-local error message for last failed call */
CAIDE_API const char* caide_last_error_message(void);
CAIDE_API CaideError caide_last_error_code(void);

/* ============================================================
 * Opaque Handle Types
 * ============================================================ */

typedef struct CaideContext_t*    CaideContext;
typedef struct CaideDocument_t*   CaideDocument;
typedef struct CaideShape_t*      CaideShape;
typedef struct CaideSketch_t*     CaideSketch;
typedef struct CaideMesh_t*       CaideMesh;
typedef struct CaideReference_t*  CaideReference;

/* ============================================================
 * Context (Global State)
 * ============================================================ */

CAIDE_API CaideError caide_context_create(CaideContext* out_ctx);
CAIDE_API void       caide_context_destroy(CaideContext ctx);

/* ============================================================
 * Document (One per design session)
 * ============================================================ */

CAIDE_API CaideError caide_document_create(CaideContext ctx, CaideDocument* out_doc);
CAIDE_API void       caide_document_destroy(CaideDocument doc);
CAIDE_API CaideError caide_document_undo(CaideDocument doc);
CAIDE_API CaideError caide_document_redo(CaideDocument doc);
CAIDE_API int        caide_document_history_count(CaideDocument doc);
CAIDE_API CaideError caide_document_get_shape(CaideDocument doc, const char* ref_id, CaideShape* out_shape);

/* ============================================================
 * Geometry Primitives
 * ============================================================ */

CAIDE_API CaideError caide_shape_create_box(CaideDocument doc, double width, double height, double depth, CaideShape* out_shape);
CAIDE_API CaideError caide_shape_create_cylinder(CaideDocument doc, double radius, double height, CaideShape* out_shape);
CAIDE_API CaideError caide_shape_create_sphere(CaideDocument doc, double radius, CaideShape* out_shape);
CAIDE_API CaideError caide_shape_create_cone(CaideDocument doc, double radius1, double radius2, double height, CaideShape* out_shape);
CAIDE_API CaideError caide_shape_create_torus(CaideDocument doc, double major_radius, double minor_radius, CaideShape* out_shape);

CAIDE_API void caide_shape_destroy(CaideShape shape);
CAIDE_API CaideError caide_shape_clone(CaideShape shape, CaideShape* out_clone);
CAIDE_API int caide_shape_is_valid(CaideShape shape);

/* Bounding box */
CAIDE_API CaideError caide_shape_bounding_box(CaideShape shape,
    double* out_min_x, double* out_min_y, double* out_min_z,
    double* out_max_x, double* out_max_y, double* out_max_z);

/* Volume and area */
CAIDE_API CaideError caide_shape_volume(CaideShape shape, double* out_volume);
CAIDE_API CaideError caide_shape_surface_area(CaideShape shape, double* out_area);

/* ============================================================
 * Boolean Operations
 * ============================================================ */

CAIDE_API CaideError caide_boolean_union(CaideShape shape_a, CaideShape shape_b, CaideShape* out_result);
CAIDE_API CaideError caide_boolean_subtract(CaideShape shape_a, CaideShape shape_b, CaideShape* out_result);
CAIDE_API CaideError caide_boolean_intersect(CaideShape shape_a, CaideShape shape_b, CaideShape* out_result);

/* ============================================================
 * Transformations
 * ============================================================ */

CAIDE_API CaideError caide_transform_translate(CaideShape shape, double dx, double dy, double dz, CaideShape* out_result);
CAIDE_API CaideError caide_transform_rotate(CaideShape shape, double axis_x, double axis_y, double axis_z, double angle_deg, CaideShape* out_result);
CAIDE_API CaideError caide_transform_scale(CaideShape shape, double cx, double cy, double cz, double factor, CaideShape* out_result);
CAIDE_API CaideError caide_transform_mirror(CaideShape shape, double plane_x, double plane_y, double plane_z, double plane_d, CaideShape* out_result);

/* ============================================================
 * Feature Operations
 * ============================================================ */

CAIDE_API CaideError caide_feature_fillet(CaideShape shape, double radius, const int* edge_indices, int edge_count, CaideShape* out_result);
CAIDE_API CaideError caide_feature_chamfer(CaideShape shape, double distance, const int* edge_indices, int edge_count, CaideShape* out_result);
CAIDE_API CaideError caide_feature_shell(CaideShape shape, double thickness, const int* face_indices, int face_count, CaideShape* out_result);
CAIDE_API CaideError caide_feature_draft(CaideShape shape, double angle_deg, double dir_x, double dir_y, double dir_z, const int* face_indices, int face_count, CaideShape* out_result);

/* ============================================================
 * Sketch & Wire Operations
 * ============================================================ */

CAIDE_API CaideError caide_sketch_create(CaideDocument doc, CaideSketch* out_sketch);
CAIDE_API void       caide_sketch_destroy(CaideSketch sketch);
CAIDE_API CaideError caide_sketch_add_line(CaideSketch sketch, double x1, double y1, double x2, double y2);
CAIDE_API CaideError caide_sketch_add_arc(CaideSketch sketch, double cx, double cy, double radius, double start_deg, double end_deg);
CAIDE_API CaideError caide_sketch_add_circle(CaideSketch sketch, double cx, double cy, double radius);
CAIDE_API CaideError caide_sketch_add_spline(CaideSketch sketch, const double* points, int point_count);
CAIDE_API CaideError caide_sketch_close(CaideSketch sketch);
CAIDE_API CaideError caide_sketch_to_face(CaideSketch sketch, CaideShape* out_face);
CAIDE_API CaideError caide_sketch_to_wire(CaideSketch sketch, CaideShape* out_wire);

/* ============================================================
 * Advanced Shape Operations
 * ============================================================ */

CAIDE_API CaideError caide_shape_extrude(CaideShape profile, double dx, double dy, double dz, CaideShape* out_result);
CAIDE_API CaideError caide_shape_revolve(CaideShape profile, double axis_x, double axis_y, double axis_z, double angle_deg, CaideShape* out_result);
CAIDE_API CaideError caide_shape_sweep(CaideShape profile, CaideShape spine, CaideShape* out_result);
CAIDE_API CaideError caide_shape_loft(const CaideShape* profiles, int profile_count, CaideShape* out_result);

/* ============================================================
 * Mesh Export
 * ============================================================ */

typedef enum {
    CAIDE_FORMAT_STL_BINARY = 0,
    CAIDE_FORMAT_STL_ASCII = 1,
    CAIDE_FORMAT_OBJ = 2,
    CAIDE_FORMAT_3MF = 3,
    CAIDE_FORMAT_STEP = 4,
    CAIDE_FORMAT_GLTF = 5,
} CaideExportFormat;

typedef struct {
    double linear_deflection;   /* default: 0.1 */
    double angular_deflection;  /* default: 0.5 (radians) */
    int    relative;            /* 0 = absolute, 1 = relative to shape size */
} CaideTessellationParams;

CAIDE_API CaideError caide_export_to_file(CaideShape shape, CaideExportFormat format, const char* file_path, const CaideTessellationParams* params);
CAIDE_API CaideError caide_export_to_buffer(CaideShape shape, CaideExportFormat format, const CaideTessellationParams* params, uint8_t** out_buffer, size_t* out_size);
CAIDE_API void       caide_buffer_free(uint8_t* buffer);

/* Tessellation access (for renderers) */
CAIDE_API CaideError caide_mesh_tessellate(CaideShape shape, const CaideTessellationParams* params, CaideMesh* out_mesh);
CAIDE_API void       caide_mesh_destroy(CaideMesh mesh);
CAIDE_API int        caide_mesh_vertex_count(CaideMesh mesh);
CAIDE_API int        caide_mesh_triangle_count(CaideMesh mesh);
CAIDE_API CaideError caide_mesh_get_vertices(CaideMesh mesh, float** out_vertices, int* out_count);
CAIDE_API CaideError caide_mesh_get_normals(CaideMesh mesh, float** out_normals, int* out_count);
CAIDE_API CaideError caide_mesh_get_indices(CaideMesh mesh, uint32_t** out_indices, int* out_count);

/* ============================================================
 * Printability Analysis
 * ============================================================ */

typedef enum {
    CAIDE_PRINT_OK = 0,          /* Green — no issues */
    CAIDE_PRINT_WARNING = 1,     /* Yellow — may need support */
    CAIDE_PRINT_CRITICAL = 2,    /* Red — will likely fail */
} CaidePrintSeverity;

typedef struct {
    int face_index;
    CaidePrintSeverity severity;
    const char* reason;         /* e.g., "overhang 62°" */
    double value;               /* overhang angle, wall thickness, etc. */
} CaidePrintIssue;

typedef struct {
    double overhang_threshold_deg;  /* default: 45.0 */
    double min_wall_thickness_mm;   /* default: 0.4 */
    double min_feature_size_mm;     /* default: 0.2 */
    double bridge_max_length_mm;    /* default: 10.0 */
    double build_dir_x, build_dir_y, build_dir_z;  /* default: 0,0,1 (Z-up) */
} CaidePrintAnalysisParams;

CAIDE_API CaideError caide_print_analyze(CaideShape shape, const CaidePrintAnalysisParams* params,
    CaidePrintIssue** out_issues, int* out_issue_count);
CAIDE_API void       caide_print_issues_free(CaidePrintIssue* issues, int count);

CAIDE_API CaideError caide_print_wall_thickness(CaideShape shape, double* out_min_thickness);
CAIDE_API CaideError caide_print_support_volume(CaideShape shape, const CaidePrintAnalysisParams* params, double* out_volume);
CAIDE_API CaideError caide_print_bed_adhesion_area(CaideShape shape, double* out_area);

/* ============================================================
 * Command Executor (High-Level API)
 * ============================================================ */

typedef struct {
    const char* type;           /* e.g., "create_box", "boolean_union" */
    const char* params_json;    /* JSON object with command parameters */
    const char* target_ref;     /* target shape reference (may be NULL) */
} CaideCommand;

CAIDE_API CaideError caide_command_execute(CaideDocument doc, const CaideCommand* cmd, CaideShape* out_result);
CAIDE_API CaideError caide_command_execute_batch(CaideDocument doc, const CaideCommand* cmds, int cmd_count);
CAIDE_API CaideError caide_command_validate(const CaideCommand* cmd);

/* ============================================================
 * Stable References
 * ============================================================ */

CAIDE_API CaideError caide_reference_create(CaideShape shape, const char* label, CaideReference* out_ref);
CAIDE_API void       caide_reference_destroy(CaideReference ref);
CAIDE_API CaideError caide_reference_resolve(CaideDocument doc, const char* ref_id, CaideShape* out_shape);
CAIDE_API const char* caide_reference_get_id(CaideReference ref);
CAIDE_API int        caide_reference_is_valid(CaideReference ref);

#ifdef __cplusplus
}
#endif

#endif /* CAIDE_CORE_H */
