#ifndef TX_3D_H
#define TX_3D_H

#include <TX/ECS.h>
#include <TX/TX_Shade.h>
#include <TX/Collections/List.h>
#include <TX/Math/linalg.h>
#include <TX/Math/stride.h>

typedef enum TX_RasterizerFlags {
    TX_RASTERIZER_ALPHA_SCISSOR       = 1 << 0,
    TX_RASTERIZER_CULL_BACKFACE       = 1 << 1,
    TX_RASTERIZER_INTERPOLATE_NORMALS = 1 << 2,
    TX_RASTERIZER_TEST_DEPTH          = 1 << 3,
    TX_RASTERIZER_WRITE_DEPTH         = 1 << 4,
    TX_RASTERIZER_SORT_TRIANGLES      = 1 << 5,
    TX_RASTERIZER_FLAG_COMBINATIONS   = 1 << 6,
} TX_RasterizerFlags;

typedef struct TX_Mesh TX_Mesh;
typedef struct TX_Sculpture TX_Sculpture;
typedef struct TX_PolyChain TX_PolyChain;
typedef struct TX_WorldGeometry TX_WorldGeometry;
typedef struct TX_RenderInstance TX_RenderInstance;
typedef struct TX_World TX_World;
typedef struct TX_Model TX_Model;
typedef struct TX_ModelInstance TX_ModelInstance;
typedef struct TX_RasterThreadPool TX_RasterThreadPool;
typedef struct TX_TriangleDraw TX_TriangleDraw;

typedef xform3 (*TX_TransformCompositor)(ECS_Handle *self, xform3 lhs);
typedef sd_vec2 (*TX_VertexProjector)(ECS_Handle *self, sd_vec3 pos, sd_vec2 midpoint);
typedef void (*TX_RasterScanner)(ECS_Handle *self, TX_CanvasTile tile, TX_TriangleDraw *triangle, int triangle_bounds[2]);

typedef struct TX_MeshFace {
    size_t idx_verts[3];
    size_t idx_tverts[3];
} TX_MeshFace;

typedef struct TX_TriangleDraw {
    TX_ShaderCall *shader_pipeline;
    vec3 vs_verts[3];
    vec3 vs_nrmls[3];
    vec2 ts_verts[3];
    vec2 ss_verts[3];
    int left, right;
    int top, bottom;
    TX_RasterizerFlags flags;
} TX_TriangleDraw;

typedef struct TX_Rasterizer {
    ECS_Handle *world;
    ECS_Handle *target;
    List(TX_TriangleDraw) *triangles;
    TX_RasterThreadPool *thread_pool;
    TX_VertexProjector project;
    TX_RasterScanner scan;
    float near;
} TX_Rasterizer;

typedef struct TX_ParallelProjector {
    vec2 slope;
    vec2 scale;
} TX_ParallelProjector;

typedef struct TX_PerspectiveFOV {
    float fov;
    float tan_half_fov;
} TX_PerspectiveFOV;

typedef struct TX_ModelArgs {
    TX_Mesh *(*get_mesh)(ECS_Handle *self);
} TX_ModelArgs;

typedef struct TX_ModelInstanceArgs {
    ECS_Component(TX_ShaderCall) **shader_components;
    size_t render_batch;
    TX_RasterizerFlags flags;
} TX_ModelInstanceArgs;

typedef struct TX_Teapot {
    float scale;
} TX_Teapot;

typedef struct TX_Torus {
    size_t outer_precision, inner_precision;
    float outer_radius, inner_radius;
} TX_Torus;

typedef struct TX_Sphere {
    size_t nrings, ring_precision;
    float radius;
} TX_Sphere;

typedef struct TX_Rect {
    float width, height;
} TX_Rect;

typedef struct TX_Cubemap {
    float scale;
} TX_Cubemap;

struct TX_3D {
    ECS_Component(vec3) *Position;
    ECS_Component(mat3x3) *Basis;
    ECS_Component(TX_TransformCompositor) *TransformCompositor;

    ECS_Component(TX_World) *World;
    ECS_Component(TX_Model) *Model;
    ECS_Component(TX_ModelInstance) *ModelInstance;

    ECS_Component(TX_Rasterizer) *Rasterizer;
    ECS_Component(TX_ParallelProjector) *ParallelProjector;
    ECS_Component(TX_PerspectiveFOV) *PerspectiveFOV;

    ECS_Component(TX_Mesh *) *MeshPrimitive;
    ECS_Component(TX_Teapot) *Teapot;
    ECS_Component(TX_Torus) *Torus;
    ECS_Component(TX_Sphere) *Sphere;
    ECS_Component(TX_Rect) *Rect;
    ECS_Component(TX_Cubemap) *Cubemap;
};

extern struct TX_3D TX_3D;

TX_Mesh *TX_GetTeapotMesh(ECS_Handle *self);
TX_Mesh *TX_GetTorusMesh(ECS_Handle *self);
TX_Mesh *TX_GetSphereMesh(ECS_Handle *self);
TX_Mesh *TX_GetRectMesh(ECS_Handle *self);
TX_Mesh *TX_GetCubemapMesh(ECS_Handle *self);

xform3 TX_GetEntityTransform(ECS_Handle *self);
void TX_TransformEntity(ECS_Handle *self, xform3 lhs);

xform3 TX_ComposeTransformDefault(ECS_Handle *self, xform3 lhs);
xform3 TX_ComposeTransformBillboard(ECS_Handle *self, xform3 lhs);
xform3 TX_ComposeTransformCubemap(ECS_Handle *self, xform3 lhs);
xform3 TX_ComposeTransformAbsolute(ECS_Handle *self, xform3 lhs);

SD_DECLARE(TX_Mesh *, TX_CreateMesh, vec3 *, ws_verts, vec3 *, ws_nrmls, vec2 *, ts_verts, TX_MeshFace *, faces, size_t, nverts, size_t, nts_verts, size_t, nfaces)
void TX_FreeMesh(TX_Mesh *mesh);

void TX_JoinPolyChains(TX_Sculpture *sculpture, TX_PolyChain *pc1, TX_PolyChain *pc2);
TX_PolyChain *TX_SculptVertex(TX_Sculpture *sculpture, vec3 pos);
TX_PolyChain *TX_SculptEllipse(TX_Sculpture *sculpture, vec3 center, vec3 axis1, vec3 axis2, size_t precision);
TX_Mesh *TX_SculptureToMesh(TX_Sculpture *sculpture);
TX_Sculpture *TX_CreateSculpture(void);
void TX_FreeSculpture(TX_Sculpture *sculpture);

SD_DECLARE(TX_WorldGeometry *, TX_RegisterGeometry, ECS_Handle *, self, TX_Mesh *, mesh)

void TX_FreeRenderInstance(TX_RenderInstance *instance);

TX_RenderInstance *TX_InstanceWorldGeometry(TX_WorldGeometry *geometry, TX_ShaderCall *shader_pipeline, size_t render_batch, TX_RasterizerFlags flags);
void TX_FreeWorldGeometry(TX_WorldGeometry *geometry);

SD_DECLARE(sd_vec2, TX_ProjectParallel, ECS_Handle *, self, sd_vec3, point, sd_vec2, midpoint)
SD_DECLARE_VOID_RETURN(TX_ScanLinear, ECS_Handle *, self, TX_CanvasTile, tile, TX_TriangleDraw *, triangle, int [2], triangle_bounds)

SD_DECLARE(sd_vec2, TX_ProjectPerspective, ECS_Handle *, self, sd_vec3, point, sd_vec2, midpoint)
SD_DECLARE_VOID_RETURN(TX_ScanPerspective, ECS_Handle *, self, TX_CanvasTile, tile, TX_TriangleDraw *, triangle, int [2], triangle_bounds)

void TX_SetPerspectiveFOV(ECS_Handle *self, float fov);

#endif /* TX_3D_H */
