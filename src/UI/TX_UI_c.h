#ifndef TX_UI_C_H
#define TX_UI_C_H

#include <SDL3/SDL.h>
#include <TX/ECS.h>
#include <TX/TX_UI.h>
#include <TX/Math/linalg.h>

typedef struct TX_InputState {
    bool (*prev)[SDL_SCANCODE_COUNT];
    bool (*curr)[SDL_SCANCODE_COUNT];
    vec2 mouse_motion;
    vec2 wheel_motion;
} TX_InputState;

void TX_NotifyInputState(ECS_Handle *e, SDL_Event *ev);
void TX_AdvanceInputState(ECS_Handle *e);

void TX_InitInputState(void *component, void *args);
void TX_FreeInputState(void *component);

void TX_RegisterUIToECS(ECS *ecs);

#endif /* TX_UI_C_H */
