#include <TX/ECS.h>
#include <TX/TX_ECS.h>

#include "TX_UI_c.h"

struct TX_UI TX_UI;

void TX_RegisterUIToECS(ECS *ecs) {
    TX_UI.InputState = ECS_RegisterComponent(ecs, TX_InputState, {
        .init = TX_InitInputState,
        .free = TX_FreeInputState
    });

    ECS_RegisterSystem(TX_SystemGroups.SDLEvent, TX_NotifyInputState, TX_UI.InputState);
    ECS_RegisterSystem(TX_SystemGroups.PostUpdate, TX_AdvanceInputState, TX_UI.InputState);
}
