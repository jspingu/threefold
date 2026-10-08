#include <SDL3/SDL.h>
#include <TX/ECS.h>
#include <TX/Math/stride.h>
#include <TX/gamma.h>

#include "TX_Bitmap_c.h"

static constexpr int MIN_TILE_SIZE = 128;

static void PresentSubCanvas(TX_CanvasWorkerData *wd) {
    TX_Canvas *canvas = ECS_GetComponent(wd->canvas, TX_Bitmap.Canvas);
    int qot = sd_qot(canvas->width);
    int rem = sd_rem(canvas->width);

    for (int i = wd->start; i < wd->end; ++i) {
        int sd_base = i * sd_bounding_length(canvas->width);
        int32_t *pixel_base = (int32_t *)(wd->pixels + i * wd->pitch);

        for (int j = 0; j < qot; ++j) {
            sd_vec3 col = sd_vec3_load(canvas->color, sd_base + j);
            col = sd_vec3_clamp(col, sd_float_zero(), sd_float_one());
            col = sd_vec3_muls(col, sd_float_set(0xFFFF));

            sd_int byte = sd_int_set(0xFF);

            sd_int r = sd_float_to_int(sd_vx(col));
                   r = sd_int_gather_u8(gamma_encode_lut, r);
                   r = sd_int_shl(r, 16);
            sd_int g = sd_float_to_int(sd_vy(col));
                   g = sd_int_gather_u8(gamma_encode_lut, g);
                   g = sd_int_and(g, byte);
                   g = sd_int_shl(g, 8);
            sd_int b = sd_float_to_int(sd_vz(col));
                   b = sd_int_gather_u8(gamma_encode_lut, b);
                   b = sd_int_and(b, byte);

            sd_int out = sd_int_or(r, sd_int_or(g, b));
            sd_int_storeu(pixel_base + j * sd_length(), out);
        }

        for (int j = 0; j < rem; ++j) {
            sd_vec3_scalar col = sd_vec3_loads(canvas->color, (sd_base + qot) * sd_length() + j);

            uint16_t r = SDL_clamp(col.x, 0, 1) * 0xFFFF;
                     r = gamma_encode_lut[r];
            uint16_t g = SDL_clamp(col.y, 0, 1) * 0xFFFF;
                     g = gamma_encode_lut[g];
            uint16_t b = SDL_clamp(col.z, 0, 1) * 0xFFFF;
                     b = gamma_encode_lut[b];

            pixel_base[qot * sd_length() + j] = (r << 16) | (g << 8) | b;
        }
    }
}

int SD_VARIANT(TX_CanvasWorker)(void *data) {
    TX_CanvasWorkerData *wd = data;
loop:
    SDL_WaitSemaphore(wd->worker_wake);

    if (wd->exit)
        goto exit;

    PresentSubCanvas(wd);
    SDL_SignalSemaphore(wd->worker_rest);
    goto loop;
exit:
    return 0;
}

void SD_VARIANT(TX_PresentCanvas)(ECS_Handle *self) {
    TX_Canvas *canvas = ECS_GetComponent(self, TX_Bitmap.Canvas);
    TX_Viewport *vp = ECS_GetComponent(self, TX_Bitmap.Viewport);
    void *pixels;
    int pitch;

    SDL_LockTexture(vp->texture, nullptr, (void **)&pixels, &pitch);
    TX_CanvasThreadPool *pool = canvas->thread_pool;
    int nproc = SDL_GetNumLogicalCPUCores();
    int qot = canvas->height / nproc;
    int rem = canvas->height % nproc;

    for (int i = 0; i < nproc; ++i) {
        pool->worker_data[i].pixels = pixels;
        pool->worker_data[i].pitch = pitch;
        pool->worker_data[i].start = i * qot + SDL_min(i, rem);
        pool->worker_data[i].end = (i + 1) * qot + SDL_min(i + 1, rem);
    }

    for (int i = 0; i < nproc; ++i)
        SDL_SignalSemaphore(pool->worker_data[i].worker_wake);

    for (int i = 0; i < nproc; ++i)
        SDL_WaitSemaphore(pool->worker_rest);

    SDL_UnlockTexture(vp->texture);
    SDL_RenderTexture(vp->renderer, vp->texture, nullptr, nullptr);
    SDL_RenderPresent(vp->renderer);
}

void SD_VARIANT(TX_InitCanvas)(void *component, void *args) {
    TX_Canvas *canvas = component, *cargs = args;
    canvas->width = cargs->width;
    canvas->height = cargs->height;

    size_t sd_size = sd_bounding_size(canvas->width) * canvas->height;
    canvas->color = SDL_aligned_alloc(SD_ALIGN, sd_size * 3);
    canvas->depth = SDL_aligned_alloc(SD_ALIGN, sd_size);
    
    int tile_size = SDL_max(MIN_TILE_SIZE, sd_length());
    int htiles = (canvas->width + tile_size - 1) / tile_size;
    int vtiles = (canvas->height + tile_size - 1) / tile_size;
    canvas->ntiles = htiles * vtiles;
    canvas->tiles = SDL_malloc(sizeof(TX_CanvasTile) * canvas->ntiles);

    for (int i = 0; i < vtiles; ++i)
        for (int j = 0; j < htiles; ++j)
            canvas->tiles[i * htiles + j] = (TX_CanvasTile) {
                .left = j * tile_size,
                .right = SDL_min((j + 1) * tile_size, canvas->width),
                .top = i * tile_size,
                .bottom = SDL_min((i + 1) * tile_size, canvas->height),
                .scanlines = {
                    SDL_aligned_alloc(SD_ALIGN, sizeof(int32_t) * tile_size),
                    SDL_aligned_alloc(SD_ALIGN, sizeof(int32_t) * tile_size)
                }
            };
}

#ifndef SD_SRC_VARIANT

void TX_AttachCanvas(ECS_Handle *self, ECS_Component(void) *component) {
    TX_Canvas *canvas = ECS_GetComponent(self, component);
    TX_CanvasThreadPool *pool = SDL_malloc(sizeof(TX_CanvasThreadPool));
    int nproc = SDL_GetNumLogicalCPUCores();
    canvas->thread_pool = pool;
    pool->threads = SDL_malloc(sizeof(SDL_Thread *) * nproc);
    pool->worker_data = SDL_malloc(sizeof(TX_CanvasWorkerData) * nproc);
    pool->worker_rest = SDL_CreateSemaphore(0);

    for (int i = 0; i < nproc; ++i) {
        pool->worker_data[i] = (TX_CanvasWorkerData) {
            .canvas = self,
            .worker_wake = SDL_CreateSemaphore(0),
            .worker_rest = pool->worker_rest,
            .exit = false
        };

        pool->threads[i] = SDL_CreateThread(SD_SELECT(TX_CanvasWorker), "canvasworker", pool->worker_data + i);
    }
}

void TX_DetachCanvas(ECS_Handle *self, ECS_Component(void) *component) {
    TX_Canvas *canvas = ECS_GetComponent(self, component);
    TX_CanvasThreadPool *pool = canvas->thread_pool;
    int nproc = SDL_GetNumLogicalCPUCores();

    for (int i = 0; i < nproc; ++i)
        pool->worker_data[i].exit = true;

    for (int i = 0; i < nproc; ++i)
        SDL_SignalSemaphore(pool->worker_data[i].worker_wake);

    for (int i = 0; i < nproc; ++i) {
        SDL_WaitThread(pool->threads[i], nullptr);
        SDL_DestroySemaphore(pool->worker_data[i].worker_wake);
    }

    SDL_DestroySemaphore(pool->worker_rest);
    SDL_free(pool->worker_data);
    SDL_free(pool->threads);
    SDL_free(pool);
}

void TX_FreeCanvas(void *component) {
    TX_Canvas *canvas = component;

    SDL_aligned_free(canvas->color);
    SDL_aligned_free(canvas->depth);

    SDL_Log("hello?");

    for (int i = 0; i < canvas->ntiles; ++i) {
        SDL_aligned_free(canvas->tiles[i].scanlines[0]);
        SDL_aligned_free(canvas->tiles[i].scanlines[1]);
    }

    SDL_free(canvas->tiles);
}

#endif /* SD_SRC_VARIANT */
