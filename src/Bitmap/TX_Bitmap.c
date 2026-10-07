#include <TX/ECS.h>
#include <TX/TX_ECS.h>
#include <TX/Math/stride.h>

#include "TX_Bitmap_c.h"

void TX_RegisterBitmapToECS(ECS *ecs) {
    TX_Components.Canvas = ECS_RegisterComponent(ecs, TX_Canvas, {
        .attach = TX_AttachCanvas,
        .detach = TX_DetachCanvas,
        .init = SD_SELECT(TX_InitCanvas),
        .free = TX_FreeCanvas
    });

    TX_Components.Viewport = ECS_RegisterComponent(ecs, TX_Viewport, {
        .init = TX_InitViewport,
        .free = TX_FreeViewport
    });

    TX_Components.TextureBank = ECS_RegisterComponent(ecs, TX_ResourceBank, {
        .attach = TX_AttachTextureBank,
        .detach = TX_DetachResourceBank
    });

    TX_Components.CubemapBank = ECS_RegisterComponent(ecs, TX_ResourceBank, {
        .attach = TX_AttachCubemapBank,
        .detach = TX_DetachResourceBank
    });

    ECS_RegisterSystem(TX_SystemGroups.RenderPresent, SD_SELECT(TX_PresentCanvas), TX_Components.Viewport, TX_Components.Canvas);
}

