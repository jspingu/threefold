#ifndef TX_BITMAP_C_H
#define TX_BITMAP_C_H

#include <TX/TX_Bitmap.h>
#include <TX/Collections/Strmap.h>

typedef struct TX_CanvasWorkerData {
    ECS_Handle *canvas;
    SDL_Semaphore *worker_wake;
    SDL_Semaphore *worker_rest;
    unsigned char *pixels;
    int pitch;
    int start, end;
    bool exit;
} TX_CanvasWorkerData;

typedef struct TX_CanvasThreadPool {
    SDL_Thread **threads;
    TX_CanvasWorkerData *worker_data;
    SDL_Semaphore *worker_rest;
} TX_CanvasThreadPool;

void TX_InitViewport(void *component, void *args);
void TX_FreeViewport(void *component);

SD_DECLARE_VOID_RETURN(TX_PresentCanvas, ECS_Handle *, self)
SD_DECLARE_VOID_RETURN(TX_InitCanvas, void *, component, void *, args)
void TX_AttachCanvas(ECS_Handle *self, ECS_Component(void) *component);
void TX_DetachCanvas(ECS_Handle *self, ECS_Component(void) *component);
void TX_FreeCanvas(void *component);

SD_DECLARE(int, TX_CanvasWorker, void *, data)

void TX_AttachTextureBank(ECS_Handle *self, ECS_Component(void) *component);
void TX_AttachCubemapBank(ECS_Handle *self, ECS_Component(void) *component);

void TX_RegisterBitmapToECS(ECS *ecs);

#endif /* TX_BITMAP_C_H */
