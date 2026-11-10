#include <SDL3/SDL.h>
#include <TX/ECS.h>
#include <TX/TX_ECS.h>
#include <TX/TX_3D.h>
#include <TX/TX_Bitmap.h>
#include <TX/TX_UI.h>
#include <TX/Math/linalg.h>
#include <TX/Math/stride.h>

#include "LocalComponents.h"

char *SD_VARIANT(GetSIMDBackend)(void) {
#ifdef __AVX512F__
    return "avx512f";
#elifdef __AVX2__
    return "avx2";
#elifdef __SSE2__
    return "sse2";
#elifdef __ARM_FEATURE_SVE
    return "sve";
#elifdef __ARM_NEON
    return "neon";
#elifdef __riscv_v
    return "rvv";
#else
    return "scalar";
#endif
}

#ifndef SD_SRC_VARIANT

SD_DECLARE(char *, GetSIMDBackend)

struct LocalComponents Components;

static bool vec3_eq(vec3 lhs, vec3 rhs) {
    return lhs.x == rhs.x &&
           lhs.y == rhs.y &&
           lhs.z == rhs.y;
}

void UpdateGrabMouse(ECS_Handle *self, double delta) {
    (void)delta;
    bool *grabbed = ECS_GetComponent(self, Components.GrabMouse);
    ECS_Handle *is = ECS_GetAncestorWithComponent(self, TX_UI.InputState, true);
    ECS_Handle *root = ECS_GetAncestorWithComponent(self, TX_Bitmap.Window, true);
    TX_Window *window = ECS_GetComponent(root, TX_Bitmap.Window);
    
    if (TX_IsKeyJustDown(is, SDL_SCANCODE_ESCAPE)) {
        *grabbed = !*grabbed;       
        SDL_SetWindowRelativeMouseMode(window->window, *grabbed);
    }
}

void UpdateFreeCam(ECS_Handle *self, double delta) {
    ECS_Handle *mouse_grab = ECS_GetAncestorWithComponent(self, Components.GrabMouse, true);
    ECS_Handle *is = ECS_GetAncestorWithComponent(self, TX_UI.InputState, true);

    if (!*ECS_GetComponent(mouse_grab, Components.GrabMouse))
        return;

    FreeCam *cam = ECS_GetComponent(self, Components.FreeCam);
    vec3 *pos = ECS_GetComponent(self, TX_3D.Position);
    mat3x3 *basis = ECS_GetComponent(self, TX_3D.Basis);

    vec3 input_axis = vec3_zero;

    if (TX_IsKeyDown(is, SDL_SCANCODE_A))
        input_axis.x -= 1;
    if (TX_IsKeyDown(is, SDL_SCANCODE_D))
        input_axis.x += 1;
    if (TX_IsKeyDown(is, SDL_SCANCODE_S))
        input_axis.z -= 1;
    if (TX_IsKeyDown(is, SDL_SCANCODE_W))
        input_axis.z += 1;
    if (TX_IsKeyDown(is, SDL_SCANCODE_LSHIFT))
        input_axis.y -= 1;
    if (TX_IsKeyDown(is, SDL_SCANCODE_SPACE))
        input_axis.y += 1;

    input_axis = (vec3_eq(input_axis, vec3_zero)) ? vec3_zero : vec3_normalize(input_axis);
    input_axis = vec3_rotate(input_axis, vec3_j, cam->yaw);
    *pos = vec3_add(*pos, vec3_mul(input_axis, 250 * delta));

    vec2 mouse_motion = TX_GetMouseMotion(is);

    cam->yaw += mouse_motion.x * 0.15 * delta;
    cam->yaw = cam->yaw - SDL_floor(cam->yaw / (2 * SDL_PI_F)) * 2 * SDL_PI_F;
    cam->pitch += mouse_motion.y * 0.15 * delta;
    cam->pitch = SDL_clamp(cam->pitch, -SDL_PI_F / 2, SDL_PI_F / 2);

    ECS_Handle *fov = ECS_GetAncestorWithComponent(self, TX_3D.PerspectiveFOV, true);
    float fov_curr = ECS_GetComponent(fov, TX_3D.PerspectiveFOV)->fov;
    TX_SetPerspectiveFOV(fov, SDL_clamp(fov_curr - TX_GetWheelMotion(is).y * 16 * delta, 30 * SDL_PI_F / 180, 150 * SDL_PI_F / 180));

    *basis = mat3x3_rotate(
        mat3x3_rotate(mat3x3_identity, vec3_i, cam->pitch),
        vec3_j, cam->yaw
    );
}

void UpdateStats(ECS_Handle *self, double delta) {
    ECS_Handle *root = ECS_GetAncestorWithComponent(self, TX_Bitmap.Window, true);
    TX_Window *window = ECS_GetComponent(root, TX_Bitmap.Window);
    char (*fps_str)[64] = ECS_GetComponent(self, Components.Stats);
    char *simd_backend = GetSIMDBackend();
    SDL_snprintf(*fps_str, 64, "threefold demo | %s | %ix%i | FPS: %.2f", simd_backend, window->width, window->height, 1.0 / delta);
    TX_SetText(self, *fps_str);
}

void RegisterToECS(ECS *ecs) {
    Components.GrabMouse = ECS_RegisterComponent(ecs, bool, {});
    Components.FreeCam = ECS_RegisterComponent(ecs, FreeCam, {});
    Components.Stats = ECS_RegisterComponent(ecs, char [64], {});

    ECS_RegisterSystem(TX_SystemGroups.Update, UpdateGrabMouse, Components.GrabMouse);

    ECS_RegisterSystem(TX_SystemGroups.Update, UpdateFreeCam,
        Components.FreeCam,
        TX_3D.Position,
        TX_3D.Basis
    );

    ECS_RegisterSystem(TX_SystemGroups.Update, UpdateStats, Components.Stats, TX_UI.Text);
}

#endif
