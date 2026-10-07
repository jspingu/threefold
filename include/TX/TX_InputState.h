#ifndef TX_INPUTSTATE_H
#define TX_INPUTSTATE_H

#include <SDL3/SDL.h>
#include <TX/ECS.h>
#include <TX/Math/linalg.h>

typedef struct TX_InputState TX_InputState;

bool TX_IsKeyDown(ECS_Handle *self, SDL_Scancode sc);
bool TX_IsKeyJustDown(ECS_Handle *self, SDL_Scancode sc);
bool TX_IsKeyJustUp(ECS_Handle *self, SDL_Scancode sc);

vec2 TX_GetMouseMotion(ECS_Handle *self);
vec2 TX_GetWheelMotion(ECS_Handle *self);

#endif /* TX_INPUTSTATE_H */
