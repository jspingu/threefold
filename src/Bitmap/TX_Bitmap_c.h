#ifndef TX_BITMAP_C_H
#define TX_BITMAP_C_H

#include <SDL3/SDL.h>
#include <TX/TX_Bitmap.h>
#include <TX/Math/stride.h>

typedef struct TX_PresentWorkerData {
    SDL_Semaphore *worker_wake;
    SDL_Semaphore *worker_rest;
    unsigned char *pixels;
    TX_Canvas *canvas;
    int pitch;
    int start, end;
    bool exit;
} TX_PresentWorkerData;

typedef struct TX_PresentThreadPool {
    SDL_Thread **threads;
    TX_PresentWorkerData *worker_data;
    SDL_Semaphore *worker_rest;
} TX_PresentThreadPool;

SD_DECLARE_VOID_RETURN(TX_PresentCanvas, ECS_Handle *, self)
SD_DECLARE(int, TX_PresentWorker, void *, data)

void TX_NotifyWindow(ECS_Handle *self, SDL_Event *ev);
void TX_NotifyWindowResize(ECS_Handle *self, SDL_Event *ev);

void TX_InitWindow(void *component, void *args);
void TX_FreeWindow(void *component);

void TX_AttachWindowResize(ECS_Handle *self, ECS_Component(void) *component);

SD_DECLARE_VOID_RETURN(TX_InitCanvas, void *, component, void *, args)
void TX_FreeCanvas(void *component);

void TX_AttachTextureBank(ECS_Handle *self, ECS_Component(void) *component);
void TX_AttachCubemapBank(ECS_Handle *self, ECS_Component(void) *component);

void TX_RegisterBitmapToECS(ECS *ecs);

#endif /* TX_BITMAP_C_H */
