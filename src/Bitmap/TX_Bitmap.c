#include <TX/ECS.h>
#include <TX/TX_ECS.h>
#include <TX/Math/stride.h>

#include "TX_Bitmap_c.h"

struct TX_Bitmap TX_Bitmap;

void TX_RegisterBitmapToECS(ECS *ecs) {
    TX_Bitmap.Canvas = ECS_RegisterComponent(ecs, TX_Canvas, {
        .attach = TX_AttachCanvas,
        .detach = TX_DetachCanvas,
        .init = SD_SELECT(TX_InitCanvas),
        .free = TX_FreeCanvas
    });

    TX_Bitmap.Viewport = ECS_RegisterComponent(ecs, TX_Viewport, {
        .init = TX_InitViewport,
        .free = TX_FreeViewport
    });

    TX_Bitmap.TextureBank = ECS_RegisterComponent(ecs, TX_ResourceBank, {
        .attach = TX_AttachTextureBank,
        .detach = TX_DetachResourceBank
    });

    TX_Bitmap.CubemapBank = ECS_RegisterComponent(ecs, TX_ResourceBank, {
        .attach = TX_AttachCubemapBank,
        .detach = TX_DetachResourceBank
    });

    ECS_RegisterSystem(TX_SystemGroups.RenderPresent, SD_SELECT(TX_PresentCanvas), TX_Bitmap.Viewport, TX_Bitmap.Canvas);
}
