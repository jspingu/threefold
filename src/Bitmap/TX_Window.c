#include <SDL3/SDL.h>
#include <TX/ECS.h>
#include <TX/Math/stride.h>
#include <TX/gamma.h>

#include "TX_Bitmap_c.h"

static void PresentSubCanvas(TX_PresentWorkerData *wd) {
    TX_Canvas *canvas = wd->canvas;
    int qot = sd_qot(canvas->width);
    int rem = sd_rem(canvas->width);

    for (int i = wd->start; i < wd->end; ++i) {
        int sd_base = i * sd_bounding_length(canvas->width);
        int32_t *pixel_base = (int32_t *)(wd->pixels + i * wd->pitch);

        for (int j = 0; j < qot; ++j) {
            sd_vec3 col = sd_vec3_load(canvas->color, sd_base + j);
            col = sd_vec3_clamps(col, sd_float_zero(), sd_float_one());
            col = sd_vec3_muls(col, sd_float_set(0xFFFF));

            sd_int byte = sd_int_set(0xFF);

            sd_int r = sd_float_toi(sd_vx(col));
                   r = sd_int_gather_u8(gamma_encode_lut, r);
                   r = sd_int_shl(r, 16);
            sd_int g = sd_float_toi(sd_vy(col));
                   g = sd_int_gather_u8(gamma_encode_lut, g);
                   g = sd_int_and(g, byte);
                   g = sd_int_shl(g, 8);
            sd_int b = sd_float_toi(sd_vz(col));
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

int SD_VARIANT(TX_PresentWorker)(void *data) {
    TX_PresentWorkerData *wd = data;
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
    TX_Window *window = ECS_GetComponent(self, TX_Bitmap.Window);
    TX_Canvas *canvas = ECS_GetComponent(self, TX_Bitmap.Canvas);
    void *pixels;
    int pitch;

    SDL_LockTexture(window->texture, nullptr, (void **)&pixels, &pitch);
    TX_PresentThreadPool *pool = window->pool;
    int nproc = SDL_GetNumLogicalCPUCores();
    int qot = canvas->height / nproc;
    int rem = canvas->height % nproc;

    for (int i = 0; i < nproc; ++i) {
        pool->worker_data[i].pixels = pixels;
        pool->worker_data[i].canvas = canvas;
        pool->worker_data[i].pitch = pitch;
        pool->worker_data[i].start = i * qot + SDL_min(i, rem);
        pool->worker_data[i].end = (i + 1) * qot + SDL_min(i + 1, rem);
    }

    for (int i = 0; i < nproc; ++i)
        SDL_SignalSemaphore(pool->worker_data[i].worker_wake);

    for (int i = 0; i < nproc; ++i)
        SDL_WaitSemaphore(pool->worker_rest);

    SDL_UnlockTexture(window->texture);
    SDL_RenderTexture(window->renderer, window->texture, nullptr, nullptr);
    SDL_RenderPresent(window->renderer);
}

#ifndef SD_SRC_VARIANT

void TX_NotifyWindow(ECS_Handle *self, SDL_Event *ev) {
    TX_Window *window = ECS_GetComponent(self, TX_Bitmap.Window);
    SDL_WindowID id = SDL_GetWindowID(window->window);

    if (ev->type != SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED || ev->window.windowID != id)
        return;

    SDL_DestroyTexture(window->texture);
    window->width = ev->window.data1;
    window->height = ev->window.data2;

    window->texture = SDL_CreateTexture(
        window->renderer,
        SDL_PIXELFORMAT_BGRX32,
        SDL_TEXTUREACCESS_STREAMING,
        window->width,
        window->height
    );
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
    SDL_SetDefaultTextureScaleMode(window->renderer, SDL_SCALEMODE_PIXELART);

    window->texture = SDL_CreateTexture(
        window->renderer,
        SDL_PIXELFORMAT_BGRX32,
        SDL_TEXTUREACCESS_STREAMING,
        window->width,
        window->height
    );

    TX_PresentThreadPool *pool = SDL_malloc(sizeof(TX_PresentThreadPool));
    int nproc = SDL_GetNumLogicalCPUCores();
    window->pool = pool;
    pool->threads = SDL_malloc(sizeof(SDL_Thread *) * nproc);
    pool->worker_data = SDL_malloc(sizeof(TX_PresentWorkerData) * nproc);
    pool->worker_rest = SDL_CreateSemaphore(0);

    for (int i = 0; i < nproc; ++i) {
        pool->worker_data[i] = (TX_PresentWorkerData) {
            .worker_wake = SDL_CreateSemaphore(0),
            .worker_rest = pool->worker_rest,
            .exit = false
        };

        pool->threads[i] = SDL_CreateThread(SD_SELECT(TX_PresentWorker), "presentworker", pool->worker_data + i);
    }
}

void TX_FreeWindow(void *component) {
    TX_Window *window = component;
    TX_PresentThreadPool *pool = window->pool;
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
    SDL_DestroyTexture(window->texture);
    SDL_DestroyRenderer(window->renderer);
    SDL_DestroyWindow(window->window);
}

#endif
