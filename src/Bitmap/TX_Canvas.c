#include <SDL3/SDL.h>
#include <TX/ECS.h>
#include <TX/Math/stride.h>

#include "TX_Bitmap_c.h"

static constexpr int MIN_TILE_WIDTH = 64;

void SD_VARIANT(TX_InitCanvas)(void *component, void *args) {
    TX_Canvas *canvas = component, *cargs = args;
    canvas->width = cargs->width;
    canvas->height = cargs->height;

    size_t sd_size = sd_bounding_size(canvas->width) * canvas->height;
    canvas->color = SDL_aligned_alloc(SD_ALIGN, sd_size * 3);
    canvas->depth = SDL_aligned_alloc(SD_ALIGN, sd_size);
    
    int tile_width = SDL_max(MIN_TILE_WIDTH, sd_length());
    int htiles = (canvas->width + tile_width - 1) / tile_width;
    int vtiles = (canvas->height + tile_width - 1) / tile_width;
    canvas->ntiles = htiles * vtiles;
    canvas->tile_width = tile_width;
    canvas->tiles = SDL_malloc(sizeof(TX_CanvasTile) * canvas->ntiles);

    for (int i = 0; i < vtiles; ++i)
        for (int j = 0; j < htiles; ++j)
            canvas->tiles[i * htiles + j] = (TX_CanvasTile) {
                .triangle_idxs = List_Create(size_t),
                .scanlines = {
                    SDL_aligned_alloc(SD_ALIGN, sizeof(int32_t) * tile_width),
                    SDL_aligned_alloc(SD_ALIGN, sizeof(int32_t) * tile_width)
                },
                .left = j * tile_width,
                .right = SDL_min((j + 1) * tile_width, canvas->width),
                .top = i * tile_width,
                .bottom = SDL_min((i + 1) * tile_width, canvas->height),
            };
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
