#include <TX/ECS.h>
#include <TX/TX_ECS.h>

#include "TX_Shade_c.h"

struct TX_Shade TX_Shade;

void TX_FreeShaderCall(void *component) {
    TX_ShaderCall *shader_component = component;
    SDL_free(shader_component->state);
}

void TX_RegisterShadeToECS(ECS *ecs) {
    TX_Shade.LightEnvironment = ECS_RegisterComponent(ecs, TX_LightEnvironment *, {
        .attach = TX_AttachLightEnvironment,
        .detach = TX_DetachLightEnvironment,
        .init = TX_InitLightEnvironment,
        .free = TX_FreeLightEnvironment
    });

    TX_Shade.PointLight = ECS_RegisterComponent(ecs, TX_PointLight, {
        .attach = TX_AttachPointLight,
        .detach = TX_DetachPointLight
    });

    TX_Shade.SolidColor = ECS_RegisterComponent(ecs, TX_ShaderCall, {
        .init = TX_InitSolidColor,
        .free = TX_FreeShaderCall
    });

    TX_Shade.Checkerboard = ECS_RegisterComponent(ecs, TX_ShaderCall, {
        .init = TX_InitCheckerboard,
        .free = TX_FreeShaderCall
    });

    TX_Shade.Lighting = ECS_RegisterComponent(ecs, TX_ShaderCall, {
        .attach = TX_AttachLighting,
        .init = TX_InitLighting,
        .free = TX_FreeShaderCall
    });

    TX_Shade.Sky = ECS_RegisterComponent(ecs, TX_ShaderCall, {
        .attach = TX_AttachSky,
        .init = TX_InitSky,
        .free = TX_FreeShaderCall
    });

    TX_Shade.TextureMap = ECS_RegisterComponent(ecs, TX_ShaderCall, {
        .attach = TX_AttachTextureMap,
        .detach = TX_DetachTextureMap,
        .init = TX_InitTextureMap,
        .free = TX_FreeTextureMap
    });

    ECS_RegisterSystem(TX_SystemGroups.Transform, TX_TransformPointLight, TX_Shade.PointLight);
}
