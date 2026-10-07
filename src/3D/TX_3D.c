#include <TX/ECS.h>
#include <TX/TX_ECS.h>
#include <TX/Math/stride.h>

#include "TX_3D_c.h"

void TX_Register3DToECS(ECS *ecs) {
    TX_Components.World = ECS_RegisterComponent(ecs, TX_World, {
        .init = TX_InitWorld,
        .free = TX_FreeWorld,
    });

    TX_Components.Rasterizer = ECS_RegisterComponent(ecs, TX_Rasterizer, {
        .attach = TX_AttachRasterizer,
        .detach = TX_DetachRasterizer,
        .init = TX_InitRasterizer
    });

    TX_Components.Model = ECS_RegisterComponent(ecs, TX_Model, {
        .attach = TX_AttachModel,
        .detach = TX_DetachModel,
        .init = TX_InitModel
    });

    TX_Components.ModelInstance = ECS_RegisterComponent(ecs, TX_RenderInstance, {
        .attach = TX_AttachModelInstance,
        .detach = TX_DetachModelInstance,
        .init = TX_InitModelInstance,
        .free = TX_FreeModelInstance
    });

    TX_Components.TransformCompositor = ECS_RegisterComponent(ecs, TX_TransformCompositor, {});
    TX_Components.Position = ECS_RegisterComponent(ecs, vec3, {});
    TX_Components.Basis = ECS_RegisterComponent(ecs, mat3x3, {});
    TX_Components.ParallelProjector = ECS_RegisterComponent(ecs, TX_ParallelProjector, {});
    TX_Components.PerspectiveFOV = ECS_RegisterComponent(ecs, TX_PerspectiveFOV, { .init = TX_InitPerspectiveFOV });

    TX_Components.MeshPrimitive = ECS_RegisterComponent(ecs, TX_Mesh *, { .init = TX_InitMeshPrimitive, .free = TX_FreeMeshPrimitive });
    TX_Components.Teapot = ECS_RegisterComponent(ecs, TX_Teapot, {});
    TX_Components.Torus = ECS_RegisterComponent(ecs, TX_Torus, {});
    TX_Components.Sphere = ECS_RegisterComponent(ecs, TX_Sphere, {});
    TX_Components.Rect = ECS_RegisterComponent(ecs, TX_Rect, {});
    TX_Components.Cubemap = ECS_RegisterComponent(ecs, TX_Cubemap, {});

    TX_Components.LightEnvironment = ECS_RegisterComponent(ecs, TX_LightEnvironment *, {
        .attach = TX_AttachLightEnvironment,
        .detach = TX_DetachLightEnvironment,
        .init = TX_InitLightEnvironment,
        .free = TX_FreeLightEnvironment
    });

    TX_Components.PointLight = ECS_RegisterComponent(ecs, TX_PointLight, {
        .attach = TX_AttachPointLight,
        .detach = TX_DetachPointLight
    });

    TX_Components.SolidColor = ECS_RegisterComponent(ecs, TX_ShaderComponent, {
        .init = TX_InitSolidColor,
        .free = TX_FreeShaderComponent
    });

    TX_Components.Checkerboard = ECS_RegisterComponent(ecs, TX_ShaderComponent, {
        .init = TX_InitCheckerboard,
        .free = TX_FreeShaderComponent
    });

    TX_Components.Lighting = ECS_RegisterComponent(ecs, TX_ShaderComponent, {
        .attach = TX_AttachLighting,
        .init = TX_InitLighting,
        .free = TX_FreeShaderComponent
    });

    TX_Components.Sky = ECS_RegisterComponent(ecs, TX_ShaderComponent, {
        .attach = TX_AttachSky,
        .init = TX_InitSky,
        .free = TX_FreeShaderComponent
    });

    TX_Components.TextureMap = ECS_RegisterComponent(ecs, TX_ShaderComponent, {
        .attach = TX_AttachTextureMap,
        .detach = TX_DetachTextureMap,
        .init = TX_InitTextureMap,
        .free = TX_FreeTextureMap
    });

    ECS_RegisterSystem(TX_SystemGroups.Render, SD_SELECT(TX_RenderWorld), TX_Components.Rasterizer);
    ECS_RegisterSystem(TX_SystemGroups.Transform, TX_TransformModel, TX_Components.Model);
    ECS_RegisterSystem(TX_SystemGroups.Transform, TX_TransformPointLight, TX_Components.PointLight);
}
