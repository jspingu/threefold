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

    int new_width = (int [3]) {
        ev->window.data1,                                              // TX_WINDOW_RESIZE_EXPAND
        wider ? ev->window.data2 * aspect : ev->window.data1,          // TX_WINDOW_RESIZE_KEEP_ASPECT
        wider ? resize->base_height * new_aspect : resize->base_width  // TX_WINDOW_RESIZE_KEEP_EITHER
    }[resize->strategy];

    int new_height = (int [3]) {
        ev->window.data2,                                              // TX_WINDOW_RESIZE_EXPAND
        wider ? ev->window.data2 : ev->window.data1 / aspect,          // TX_WINDOW_RESIZE_KEEP_ASPECT
        wider ? resize->base_height : resize->base_width / new_aspect  // TX_WINDOW_RESIZE_KEEP_EITHER
    }[resize->strategy];

    ECS_ProcessEntitySystemGroup(self, TX_SystemGroups.Resize, new_width, new_height);
}

void TX_AttachWindowResize(ECS_Handle *self, ECS_Component(void) *component) {
    TX_WindowResize *resize = ECS_GetComponent(self, component);
    ECS_Handle *window = ECS_GetAncestorWithComponent(self, TX_Bitmap.Window, true);
    resize->id = SDL_GetWindowID(ECS_GetComponent(window, TX_Bitmap.Window)->window);
}
