#include <SDL3/SDL.h>
#include <TX/ECS.h>
#include <TX/Math/stride.h>

#include "TX_Bitmap_c.h"

static constexpr int MIN_TILE_WIDTH = 64;

static void TX_ResizeCanvasInternal(TX_Canvas *canvas, int width, int height) {
    size_t sd_size = sd_bounding_size(width) * height;
    int htiles = (width + canvas->tile_width - 1) / canvas->tile_width;
    int vtiles = (height + canvas->tile_width - 1) / canvas->tile_width;
    int ntiles = htiles * vtiles;

    TX_FreeCanvas(canvas);

    canvas->width = width;
    canvas->height = height;
    canvas->color = SDL_aligned_alloc(SD_ALIGN, sd_size * 3);
    canvas->depth = SDL_aligned_alloc(SD_ALIGN, sd_size);
    canvas->tiles = SDL_malloc(sizeof(TX_CanvasTile) * ntiles);
    canvas->ntiles = ntiles;

    for (int i = 0; i < vtiles; ++i)
        for (int j = 0; j < htiles; ++j)
            canvas->tiles[i * htiles + j] = (TX_CanvasTile) {
                .triangle_idxs = List_Create(size_t),
                .scanlines[0] = SDL_aligned_alloc(SD_ALIGN, sizeof(int32_t) * canvas->tile_width),
                .scanlines[1] = SDL_aligned_alloc(SD_ALIGN, sizeof(int32_t) * canvas->tile_width),
                .left = j * canvas->tile_width,
                .right = SDL_min((j + 1) * canvas->tile_width, width),
                .top = i * canvas->tile_width,
                .bottom = SDL_min((i + 1) * canvas->tile_width, height)
            };
}

void SD_VARIANT(TX_ResizeCanvas)(ECS_Handle *self, int width, int height) {
    TX_Canvas *canvas = ECS_GetComponent(self, TX_Bitmap.Canvas);
    TX_ResizeCanvasInternal(canvas, width, height);
}

void SD_VARIANT(TX_InitCanvas)(void *component, void *args) {
    TX_Canvas *canvas = component, *cargs = args;
    int tile_width = SDL_max(MIN_TILE_WIDTH, sd_length());
    canvas->tile_width = tile_width;
    canvas->color = nullptr;
    canvas->depth = nullptr;
    canvas->tiles = nullptr;
    canvas->ntiles = 0;

    TX_ResizeCanvasInternal(canvas, cargs->width, cargs->height);
}

#ifndef SD_SRC_VARIANT

void TX_FreeCanvas(void *component) {
    TX_Canvas *canvas = component;

    SDL_aligned_free(canvas->color);
    SDL_aligned_free(canvas->depth);

    for (int i = 0; i < canvas->ntiles; ++i) {
        List_Free(canvas->tiles[i].triangle_idxs);
        SDL_aligned_free(canvas->tiles[i].scanlines[0]);
        SDL_aligned_free(canvas->tiles[i].scanlines[1]);
    }

    SDL_free(canvas->tiles);
}

#endif /* SD_SRC_VARIANT */
