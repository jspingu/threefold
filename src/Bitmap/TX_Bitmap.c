#include <TX/ECS.h>
#include <TX/TX_ECS.h>
#include <TX/Math/stride.h>

#include "TX_Bitmap_c.h"

struct TX_Bitmap TX_Bitmap;

void TX_RegisterBitmapToECS(ECS *ecs) {
    TX_Bitmap.Window = ECS_RegisterComponent(ecs, TX_Window, {
        .init = TX_InitWindow,
        .free = TX_FreeWindow
    });

    TX_Bitmap.WindowResize = ECS_RegisterComponent(ecs, TX_WindowResize, {
        .attach = TX_AttachWindowResize
    });

    TX_Bitmap.Canvas = ECS_RegisterComponent(ecs, TX_Canvas, {
        .init = SD_SELECT(TX_InitCanvas),
        .free = TX_FreeCanvas
    });

    TX_Bitmap.TextureBank = ECS_RegisterComponent(ecs, TX_ResourceBank, {
        .attach = TX_AttachTextureBank,
        .detach = TX_DetachResourceBank
    });

    TX_Bitmap.CubemapBank = ECS_RegisterComponent(ecs, TX_ResourceBank, {
        .attach = TX_AttachCubemapBank,
        .detach = TX_DetachResourceBank
    });

    ECS_RegisterSystem(TX_SystemGroups.SDLEvent, TX_NotifyWindow, TX_Bitmap.Window);
    ECS_RegisterSystem(TX_SystemGroups.SDLEvent, TX_NotifyWindowResize, TX_Bitmap.WindowResize);
    ECS_RegisterSystem(TX_SystemGroups.Resize, SD_SELECT(TX_ResizeCanvas), TX_Bitmap.Canvas);
    ECS_RegisterSystem(TX_SystemGroups.RenderPresent, SD_SELECT(TX_PresentCanvas), TX_Bitmap.Window, TX_Bitmap.Canvas);
}
