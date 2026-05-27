#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "caide_core/caide_core.h"

#define CHECK(expr) do { if (!(expr)) { fprintf(stderr, "CHECK failed: %s line=%d\n", #expr, __LINE__); return 1; } } while (0)

int main(void) {
    CaideContext ctx = NULL;
    CaideDocument doc = NULL;
    CaideShape box = NULL;
    CaideShape moved = NULL;
    CaideShape clone = NULL;
    CaideMesh mesh = NULL;
    uint8_t* buffer = NULL;
    size_t size = 0;

    CHECK(caide_context_create(&ctx) == CAIDE_OK);
    CHECK(caide_document_create(ctx, &doc) == CAIDE_OK);
    CHECK(caide_shape_create_box(doc, 10.0, 20.0, 30.0, &box) == CAIDE_OK);
    CHECK(caide_shape_is_valid(box) == 1);
    CHECK(caide_shape_clone(box, &clone) == CAIDE_OK);

    {
        double volume = 0.0;
        double area = 0.0;
        CHECK(caide_shape_volume(box, &volume) == CAIDE_OK);
        CHECK(caide_shape_surface_area(box, &area) == CAIDE_OK);
        CHECK(volume > 0.0);
        CHECK(area > 0.0);
    }

    CHECK(caide_transform_translate(box, 1.0, 2.0, 3.0, &moved) == CAIDE_OK);
    CHECK(caide_mesh_tessellate(moved, NULL, &mesh) == CAIDE_OK);
    CHECK(caide_mesh_triangle_count(mesh) > 0);
    CHECK(caide_export_to_buffer(moved, CAIDE_FORMAT_OBJ, NULL, &buffer, &size) == CAIDE_OK);
    CHECK(size > 0U);

    {
        float* vertices = NULL;
        int vertex_values = 0;
        CHECK(caide_mesh_get_vertices(mesh, &vertices, &vertex_values) == CAIDE_OK);
        CHECK(vertices != NULL);
        CHECK(vertex_values > 0);
    }

    {
        CaidePrintIssue* issues = NULL;
        int count = 0;
        CaidePrintAnalysisParams params = {45.0, 0.4, 0.2, 10.0, 0.0, 0.0, 1.0};
        CHECK(caide_print_analyze(moved, &params, &issues, &count) == CAIDE_OK);
        caide_print_issues_free(issues, count);
    }

    caide_buffer_free(buffer);
    caide_mesh_destroy(mesh);
    caide_shape_destroy(clone);
    caide_shape_destroy(moved);
    caide_shape_destroy(box);
    caide_document_destroy(doc);
    caide_context_destroy(ctx);
    CHECK(strcmp(caide_version(), "1.0.0") == 0);
    return 0;
}
