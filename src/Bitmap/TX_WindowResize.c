#include <SDL3/SDL.h>
#include <TX/ECS.h>
#include <TX/TX_ECS.h>
#include <TX/TX_Bitmap.h>

void TX_NotifyWindowResize(ECS_Handle *self, SDL_Event *ev) {
    TX_WindowResize *resize = ECS_GetComponent(self, TX_Bitmap.WindowResize);

    if (ev->type != SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED || ev->window.windowID != resize->id)
        return;

    float aspect = (float)resize->base_width / resize->base_height;
    float new_aspect = (float)ev->window.data1 / ev->window.data2;
    bool wider = new_aspect > aspect;

    vec2 new_dim = (vec2 [3]) {
        { .x = ev->window.data1, .y = ev->window.data2 },                                   // TX_WINDOW_RESIZE_EXPAND
        wider ? (vec2) { .x = ev->window.data2 * aspect, .y = ev->window.data2 }            // TX_WINDOW_RESIZE_KEEP_ASPECT
              : (vec2) { .x = ev->window.data1, .y = ev->window.data1 / aspect },
        wider ? (vec2) { .x = resize->base_height * new_aspect, .y = resize->base_height }  // TX_WINDOW_RESIZE_KEEP_EITHER
              : (vec2) { .x = resize->base_width, .y = resize->base_width / new_aspect }
    }[resize->strategy];

    ECS_ProcessEntitySystemGroup(self, TX_SystemGroups.Resize, new_dim);
}

void TX_AttachWindowResize(ECS_Handle *self, ECS_Component(void) *component) {
    TX_WindowResize *resize = ECS_GetComponent(self, component);
    ECS_Handle *window = ECS_GetAncestorWithComponent(self, TX_Bitmap.Window, true);
    resize->id = SDL_GetWindowID(ECS_GetComponent(window, TX_Bitmap.Window)->window);
}
