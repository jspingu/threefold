#ifndef TX_BITMAP_C_H
#define TX_BITMAP_C_H

#include <TX/TX_Bitmap.h>
#include <TX/Collections/Strmap.h>

typedef struct TX_CanvasWorkerData {
    ECS_Handle *canvas;
    SDL_Semaphore *worker_wake;
    SDL_Semaphore *worker_rest;
    uint32_t *pixels;
    int start, end;
    bool exit;
} TX_CanvasWorkerData;

typedef struct TX_CanvasThreadPool {
    SDL_Thread **threads;
    TX_CanvasWorkerData *worker_data;
    SDL_Semaphore *worker_rest;
} TX_CanvasThreadPool;

void TX_Viewport_Init(void *component, void *args);
void TX_Viewport_Free(void *component);

SD_DECLARE_VOID_RETURN(TX_Canvas_Present, ECS_Handle *, self)
SD_DECLARE_VOID_RETURN(TX_Canvas_Init, void *, component, void *, args)
void TX_Canvas_Attach(ECS_Handle *self, ECS_Component(void) *component);
void TX_Canvas_Detach(ECS_Handle *self, ECS_Component(void) *component);
void TX_Canvas_Free(void *component);

SD_DECLARE(int, TX_CanvasWorker, void *, data)

void TX_TextureBank_Attach(ECS_Handle *self, ECS_Component(void) *component);

void TX_Bitmap_RegisterToECS(ECS *ecs);

#endif /* TX_BITMAP_C_H */
