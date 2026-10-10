#ifndef TX_3D_C_H
#define TX_3D_C_H

#include <TX/TX_3D.h>
#include <TX/Collections/List.h>
#include <TX/Math/linalg.h>
#include <TX/Math/stride.h>

typedef struct TX_Mesh {
    sd_vec3 *ws_verts;
    sd_vec3 *ws_nrmls;
    vec2 *ts_verts;
    TX_MeshFace *faces;
    size_t nverts, nfaces;
} TX_Mesh;

typedef struct TX_PolyChain {
    size_t *indices;
    size_t nindices;
} TX_PolyChain;

typedef struct TX_Sculpture {
    List(vec3) *verts;
    List(TX_MeshFace) *faces;
    List(TX_PolyChain *) *chains;
} TX_Sculpture;

typedef struct TX_WorldGeometry {
    ECS_Handle *world;
    List(TX_RenderInstance *) *instances;
    TX_Mesh *mesh;
    sd_vec3 *vs_verts;
    sd_vec3 *vs_nrmls;
    sd_vec2 *ss_verts;
    xform3 xform;
} TX_WorldGeometry;

typedef struct TX_RenderInstance {
    TX_WorldGeometry *geometry;
    TX_FragmentShader *shader_pipeline;
    void **shader_states;
    size_t nshaders;
    size_t render_batch;
    TX_RasterizerFlags flags;
} TX_RenderInstance;

typedef struct TX_FlagBatch {
    List(TX_RenderInstance *) *instances;
    List(TX_TriangleDraw) *triangles;
} TX_FlagBatch;

typedef struct TX_RenderBatch {
    TX_FlagBatch *flag_batches[TX_RASTERIZER_FLAG_COMBINATIONS];
} TX_RenderBatch;

typedef struct TX_World {
    List(TX_WorldGeometry *) *geometry;
    List(TX_RenderBatch) *render_batches;
} TX_World;

typedef struct TX_Model {
    TX_WorldGeometry *geometry;
    TX_Mesh *(*get_mesh)(ECS_Handle *self);
} TX_Model;

typedef struct TX_ModelInstance {
    TX_RenderInstance *instance;
    ECS_Component(TX_ShaderComponent) **shader_components;
    size_t nshaders;
    size_t render_batch;
    TX_RasterizerFlags flags;
} TX_ModelInstance;

typedef struct TX_RasterWorkerData {
    ECS_Handle *entity;
    TX_Rasterizer *rasterizer;
    TX_Canvas *canvas;
    TX_World *world;
    SDL_Semaphore *worker_wake;
    SDL_Semaphore *worker_rest;
    SDL_AtomicInt *tile_ref;
    bool exit;
} TX_RasterWorkerData;

typedef struct TX_RasterThreadPool {
    SDL_Thread **threads;
    TX_RasterWorkerData *worker_data;
    SDL_Semaphore *worker_wake;
    SDL_Semaphore *worker_rest;
    SDL_AtomicInt tile;
} TX_RasterThreadPool;

void TX_Register3DToECS(ECS *ecs);

void TX_AttachLightEnvironment(ECS_Handle *self, ECS_Component(void) *component);
void TX_DetachLightEnvironment(ECS_Handle *self, ECS_Component(void) *component);
void TX_InitLightEnvironment(void *component, void *args);
void TX_FreeLightEnvironment(void *component);

void TX_TransformPointLight(ECS_Handle *self, xform3 composed);
void TX_AttachPointLight(ECS_Handle *self, ECS_Component(void) *component);
void TX_DetachPointLight(ECS_Handle *self, ECS_Component(void) *component);

void TX_AttachLighting(ECS_Handle *self, ECS_Component(void) *component);
void TX_InitLighting(void *component, void *args);

void TX_AttachSky(ECS_Handle *self, ECS_Component(void) *component);
void TX_InitSky(void *component, void *args);

void TX_InitSolidColor(void *component, void *args);
void TX_InitCheckerboard(void *component, void *args);

void TX_FreeShaderComponent(void *component);

void TX_AttachTextureMap(ECS_Handle *self, ECS_Component(void) *component);
void TX_DetachTextureMap(ECS_Handle *self, ECS_Component(void) *component);
void TX_InitTextureMap(void *component, void *args);
void TX_FreeTextureMap(void *component);

void TX_InitMeshPrimitive(void *component, void *args);
void TX_FreeMeshPrimitive(void *component);

void TX_TransformModel(ECS_Handle *self, xform3 composed);
void TX_AttachModel(ECS_Handle *self, ECS_Component(void) *component);
void TX_DetachModel(ECS_Handle *self, ECS_Component(void) *component);
void TX_InitModel(void *component, void *args);

void TX_AttachModelInstance(ECS_Handle *self, ECS_Component(void) *component);
void TX_DetachModelInstance(ECS_Handle *self, ECS_Component(void) *component);
void TX_InitModelInstance(void *component, void *args);
void TX_FreeModelInstance(void *component);

void TX_InitWorld(void *component, void *args);
void TX_FreeWorld(void *component);

SD_DECLARE_VOID_RETURN(TX_RenderWorld, ECS_Handle *, self)
void TX_AttachRasterizer(ECS_Handle *self, ECS_Component(void) *component);
void TX_DetachRasterizer(ECS_Handle *self, ECS_Component(void) *component);
void TX_InitRasterizer(void *component, void *args);

SD_DECLARE(int, TX_RasterWorker, void *, data)

void TX_InitPerspectiveFOV(void *component, void *args);

#endif /* TX_3D_C_H */
