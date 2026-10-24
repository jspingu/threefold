#ifndef TX_SHADE_C_H
#define TX_SHADE_C_H

#include <TX/ECS.h>
#include <TX/TX_Shade.h>
#include <TX/Math/linalg.h>

void TX_RegisterShadeToECS(ECS *ecs);

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

void TX_AttachTextureMap(ECS_Handle *self, ECS_Component(void) *component);
void TX_DetachTextureMap(ECS_Handle *self, ECS_Component(void) *component);
void TX_InitTextureMap(void *component, void *args);
void TX_FreeTextureMap(void *component);

void TX_AttachTileMap(ECS_Handle *self, ECS_Component(void) *component);
void TX_DetachTileMap(ECS_Handle *self, ECS_Component(void) *component);
void TX_InitTileMap(void *component, void *args);
void TX_FreeTileMap(void *component);

#endif /* TX_SHADE_C_H */
