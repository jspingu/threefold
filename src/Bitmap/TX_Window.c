#include <SDL3/SDL.h>
#include <TX/ECS.h>
#include <TX/TX_Bitmap.h>

void TX_ResizeWindow(ECS_Handle *self, int width, int height) {
    TX_Window *window = ECS_GetComponent(self, TX_Bitmap.Window);
    window->width = width;
    window->height = height;

    SDL_DestroyTexture(window->texture);

    window->texture = SDL_CreateTexture(
        window->renderer,
        SDL_PIXELFORMAT_BGRX32,
        SDL_TEXTUREACCESS_STREAMING,
        width,
        height
    );

    SDL_SetTextureScaleMode(window->texture, SDL_SCALEMODE_PIXELART);
}

void TX_InitWindow(void *component, void *args) {
    TX_Window *window = component;
    TX_WindowArgs *window_args = args;

    window->width = window_args->width;
    window->height = window_args->height;

    SDL_CreateWindowAndRenderer(
        window_args->title,
        window_args->width,
        window_args->height,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_HIGH_PIXEL_DENSITY,
        &window->window,
        &window->renderer
    );

    SDL_SetRenderVSync(window->renderer, 0);

    window->texture = SDL_CreateTexture(
        window->renderer,
        SDL_PIXELFORMAT_BGRX32,
        SDL_TEXTUREACCESS_STREAMING,
        window->width,
        window->height
    );

    SDL_SetTextureScaleMode(window->texture, SDL_SCALEMODE_PIXELART);
}

void TX_FreeWindow(void *component) {
    TX_Window *window = component;
    SDL_DestroyTexture(window->texture);
    SDL_DestroyRenderer(window->renderer);
    SDL_DestroyWindow(window->window);
}
