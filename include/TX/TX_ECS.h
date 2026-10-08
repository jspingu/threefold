#ifndef TX_ECS_H
#define TX_ECS_H

#include <SDL3/SDL.h>
#include <TX/ECS.h>
#include <TX/Math/linalg.h>

typedef void (*TX_SDLEventCallback)(ECS_Handle *, SDL_Event *);
typedef void (*TX_UpdateCallback)(ECS_Handle *, double);
typedef void (*TX_PostUpdateCallback)(ECS_Handle *);
typedef void (*TX_RenderCallback)(ECS_Handle *);
typedef void (*TX_RenderPresentCallback)(ECS_Handle *);
typedef void (*TX_TransformCallback)(ECS_Handle *, xform3);

struct TX_SystemGroups {
    ECS_SystemGroup(TX_SDLEventCallback) *SDLEvent;
    ECS_SystemGroup(TX_UpdateCallback) *Update;
    ECS_SystemGroup(TX_PostUpdateCallback) *PostUpdate;
    ECS_SystemGroup(TX_RenderCallback) *Render;
    ECS_SystemGroup(TX_RenderPresentCallback) *RenderPresent;
    ECS_SystemGroup(TX_TransformCallback) *Transform;
};

extern struct TX_SystemGroups TX_SystemGroups;

void TX_RegisterToECS(ECS *ecs);

#endif /* TX_ECS_H */
