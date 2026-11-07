#include <TX/ECS.h>
#include <TX/TX_ECS.h>

#include "TX_UI_c.h"

struct TX_UI TX_UI;

void TX_RegisterUIToECS(ECS *ecs) {
    TX_UI.InputState = ECS_RegisterComponent(ecs, TX_InputState, {
        .init = TX_InitInputState,
        .free = TX_FreeInputState
    });

    TX_UI.Layout = ECS_RegisterComponent(ecs, TX_Layout, {
        .attach = TX_AttachLayout,
        .detach = TX_DetachLayout,
        .init = TX_InitLayout,
        .free = TX_FreeLayout
    });

    TX_UI.Text = ECS_RegisterComponent(ecs, TX_Text, {
        .attach = TX_AttachText,
        .detach = TX_DetachText,
        .init = TX_InitText,
        .free = TX_FreeText
    });

    ECS_RegisterSystem(TX_SystemGroups.SDLEvent, TX_NotifyInputState, TX_UI.InputState);
    ECS_RegisterSystem(TX_SystemGroups.PostUpdate, TX_AdvanceInputState, TX_UI.InputState);
    ECS_RegisterSystem(TX_SystemGroups.Layout, TX_ArrangeText, TX_UI.Text);
}
