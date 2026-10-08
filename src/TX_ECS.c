#include <TX/ECS.h>
#include <TX/TX_ECS.h>
#include <TX/Math/stride.h>

#include "3D/TX_3D_c.h"
#include "Bitmap/TX_Bitmap_c.h"
#include "UI/TX_UI_c.h"

struct TX_SystemGroups TX_SystemGroups;

void TX_RegisterToECS(ECS *ecs) {
    TX_SystemGroups.SDLEvent = ECS_RegisterSystemGroup(ecs);
    TX_SystemGroups.Update = ECS_RegisterSystemGroup(ecs);
    TX_SystemGroups.PostUpdate = ECS_RegisterSystemGroup(ecs);
    TX_SystemGroups.Render = ECS_RegisterSystemGroup(ecs);
    TX_SystemGroups.RenderPresent = ECS_RegisterSystemGroup(ecs);
    TX_SystemGroups.Transform = ECS_RegisterSystemGroup(ecs);

    TX_Register3DToECS(ecs);
    TX_RegisterBitmapToECS(ecs);
    TX_RegisterUIToECS(ecs);
}
