#ifndef TX_UI_H
#define TX_UI_H

#include <SDL3/SDL.h>
#include <TX/ECS.h>
#include <TX/Math/linalg.h>

typedef struct TX_InputState TX_InputState;

struct TX_UI {
    ECS_Component(TX_InputState) *InputState;
};

extern struct TX_UI TX_UI;

bool TX_IsKeyDown(ECS_Handle *self, SDL_Scancode sc);
bool TX_IsKeyJustDown(ECS_Handle *self, SDL_Scancode sc);
bool TX_IsKeyJustUp(ECS_Handle *self, SDL_Scancode sc);

vec2 TX_GetMouseMotion(ECS_Handle *self);
vec2 TX_GetWheelMotion(ECS_Handle *self);

#endif /* TX_UI_H */
