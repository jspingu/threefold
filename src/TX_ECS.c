#include <TX/ECS.h>
#include <TX/TX_ECS.h>
#include <TX/Math/stride.h>

#include "TX_InputState_c.h"
#include "3D/TX_3D_c.h"
#include "Bitmap/TX_Bitmap_c.h"

struct TX_Components TX_Components;
struct TX_SystemGroups TX_SystemGroups;

void TX_RegisterToECS(ECS *ecs) {
    TX_SystemGroups.SDLEvent = ECS_RegisterSystemGroup(ecs);
    TX_SystemGroups.Update = ECS_RegisterSystemGroup(ecs);
    TX_SystemGroups.PostUpdate = ECS_RegisterSystemGroup(ecs);
    TX_SystemGroups.Render = ECS_RegisterSystemGroup(ecs);
    TX_SystemGroups.RenderPresent = ECS_RegisterSystemGroup(ecs);
    TX_SystemGroups.Transform = ECS_RegisterSystemGroup(ecs);

    TX_Components.InputState = ECS_RegisterComponent(ecs, TX_InputState, {
        .init = TX_InitInputState,
        .free = TX_FreeInputState
    });

    ECS_RegisterSystem(TX_SystemGroups.SDLEvent, TX_NotifyInputState, TX_Components.InputState);
    ECS_RegisterSystem(TX_SystemGroups.PostUpdate, TX_AdvanceInputState, TX_Components.InputState);

    TX_Register3DToECS(ecs);
    TX_RegisterBitmapToECS(ecs);
}
