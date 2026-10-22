#include <TX/ECS.h>
#include <TX/TX_ECS.h>
#include <TX/Math/stride.h>

#include "TX_3D_c.h"

struct TX_3D TX_3D;

void TX_Register3DToECS(ECS *ecs) {
    TX_3D.World = ECS_RegisterComponent(ecs, TX_World, {
        .init = TX_InitWorld,
        .free = TX_FreeWorld
    });

    TX_3D.Rasterizer = ECS_RegisterComponent(ecs, TX_Rasterizer, {
        .attach = TX_AttachRasterizer,
        .detach = TX_DetachRasterizer,
        .init = TX_InitRasterizer,
        .free = TX_FreeRasterizer
    });

    TX_3D.Model = ECS_RegisterComponent(ecs, TX_Model, {
        .attach = TX_AttachModel,
        .detach = TX_DetachModel,
        .init = TX_InitModel
    });

    TX_3D.ModelInstance = ECS_RegisterComponent(ecs, TX_ModelInstance, {
        .attach = TX_AttachModelInstance,
        .detach = TX_DetachModelInstance,
        .init = TX_InitModelInstance,
        .free = TX_FreeModelInstance
    });

    TX_3D.TransformCompositor = ECS_RegisterComponent(ecs, TX_TransformCompositor, {});
    TX_3D.Position = ECS_RegisterComponent(ecs, vec3, {});
    TX_3D.Basis = ECS_RegisterComponent(ecs, mat3x3, {});
    TX_3D.ParallelProjector = ECS_RegisterComponent(ecs, TX_ParallelProjector, {});
    TX_3D.PerspectiveFOV = ECS_RegisterComponent(ecs, TX_PerspectiveFOV, { .init = TX_InitPerspectiveFOV });

    TX_3D.MeshPrimitive = ECS_RegisterComponent(ecs, TX_Mesh *, { .init = TX_InitMeshPrimitive, .free = TX_FreeMeshPrimitive });
    TX_3D.Teapot = ECS_RegisterComponent(ecs, TX_Teapot, {});
    TX_3D.Torus = ECS_RegisterComponent(ecs, TX_Torus, {});
    TX_3D.Sphere = ECS_RegisterComponent(ecs, TX_Sphere, {});
    TX_3D.Rect = ECS_RegisterComponent(ecs, TX_Rect, {});
    TX_3D.Cubemap = ECS_RegisterComponent(ecs, TX_Cubemap, {});

    ECS_RegisterSystem(TX_SystemGroups.Render, SD_SELECT(TX_RenderWorld), TX_3D.Rasterizer);
    ECS_RegisterSystem(TX_SystemGroups.Transform, TX_TransformModel, TX_3D.Model);
}
