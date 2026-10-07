#ifndef TX_BITMAP_H
#define TX_BITMAP_H

#include <SDL3/SDL.h>
#include <TX/ECS.h>
#include <TX/Math/stride.h>

typedef struct TX_CanvasThreadPool TX_CanvasThreadPool;

typedef struct TX_Viewport {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    int width, height;
} TX_Viewport;

typedef struct TX_ViewportArgs {
    char *title;
    int width, height;
} TX_ViewportArgs;

typedef struct TX_CanvasTile {
    sd_int *scanlines[2];
    int left, right;
    int top, bottom;
} TX_CanvasTile;

typedef struct TX_Canvas {
    TX_CanvasThreadPool *thread_pool;
    sd_vec3 *color;
    sd_float *depth;
    TX_CanvasTile *tiles;
    int ntiles;
    int width, height;
} TX_Canvas;

typedef struct TX_Texture {
    float *color;
    int width, height;
    int unit;
} TX_Texture;

static inline sd_vec4 TX_SampleNearest(TX_Texture *texture, sd_vec2 ts) {
    sd_float unit = sd_float_set(texture->unit);
    sd_float px_x = sd_float_clamp(sd_float_mul(sd_vx(ts), unit), sd_float_zero(), sd_float_set(texture->width - 1));
    sd_float px_y = sd_float_clamp(sd_float_mul(sd_vy(ts), unit), sd_float_zero(), sd_float_set(texture->height - 1));

    sd_int pixel_index = sd_int_add(sd_int_mul(
        sd_float_to_int(px_y),
        sd_int_set(texture->width)
    ), sd_float_to_int(px_x));

    return sd_vec4_gather(texture->color, pixel_index);
}

static inline sd_vec4 TX_SampleCubemap(TX_Texture *texture, sd_vec3 dir) {
    /* 
     * Samples a cubemap texture using a direction vector.
     * Only textures returned by TX_CubemapBank are supported.
     *
     * Expected texture layout:
     *
     * +------------+------------+------------+
     * |            |            |            |
     * |    posx    |    posy    |    posz    |
     * |  (-x, -y)  |  ( x,  y)  |  ( x, -y)  |
     * |            |            |            |
     * +------------+------------+------------+
     * |            |            |            |
     * |    negx    |    negy    |    negz    |
     * |  (-x,  y)  |  (-x,  y)  |  ( x,  y)  |
     * |            |            |            |
     * +------------+------------+------------+
     */

    sd_float unit = sd_float_set(texture->unit);
    sd_vec3 rcp = sd_vec3_rcp(dir);

    sd_vec2 zy = sd_vec2_muls(sd_vec2_create(sd_vz(dir), sd_vy(dir)), sd_vx(rcp));
    sd_vec2 xz = sd_vec2_muls(sd_vec2_create(sd_vx(dir), sd_vz(dir)), sd_vy(rcp));
    sd_vec2 xy = sd_vec2_muls(sd_vec2_create(sd_vx(dir), sd_vy(dir)), sd_vz(rcp));

    /* Select the face that is offset from the intersection point the least */
    sd_float max_zy = sd_float_max(sd_float_abs(sd_vx(zy)), sd_float_abs(sd_vy(zy)));
    sd_float max_xz = sd_float_max(sd_float_abs(sd_vx(xz)), sd_float_abs(sd_vy(xz)));
    sd_float max_xy = sd_float_max(sd_float_abs(sd_vx(xy)), sd_float_abs(sd_vy(xy)));

    sd_mask mask_zy = sd_mask_and(sd_float_lt(max_zy, max_xz), sd_float_lt(max_zy, max_xy));
    sd_mask mask_xz = sd_mask_andn(sd_float_lt(max_xz, max_xy), mask_zy);

    sd_mask across_zy = sd_mask_and(sd_float_lt(sd_vx(dir), sd_float_zero()), mask_zy);
    sd_mask across_xz = sd_mask_and(sd_float_lt(sd_vy(dir), sd_float_zero()), mask_xz);
    sd_mask across_xy = sd_mask_andn(sd_float_lt(sd_vz(dir), sd_float_zero()), sd_mask_or(mask_zy, mask_xz));

    sd_mask across = sd_mask_or(sd_mask_or(across_zy, across_xz), across_xy);
    sd_int vtile = sd_int_mask_blend(sd_int_set(0), sd_int_set(1), across);

    sd_int htile = sd_int_mask_blend(sd_int_set(2), sd_int_set(1), mask_xz);           
           htile = sd_int_mask_blend(htile, sd_int_set(0), mask_zy);

    sd_vec2 tile_coord = sd_vec2_mask_blend(sd_vec2_mask_blend(xy, xz, mask_xz), zy, mask_zy);
            tile_coord = sd_vec2_fsmadd(tile_coord, unit, sd_vec2_create(unit, unit));
            tile_coord = sd_vec2_clamp(tile_coord, sd_float_zero(), sd_float_set(texture->unit * 2 - 1));

    sd_int pixel_index = sd_int_mul(vtile, sd_int_set(texture->width * texture->unit * 2));
           pixel_index = sd_int_add(pixel_index, sd_int_mul(htile, sd_int_set(texture->unit * 2)));
           pixel_index = sd_int_add(pixel_index, sd_int_mul(sd_float_to_int(sd_vy(tile_coord)), sd_int_set(texture->width)));
           pixel_index = sd_int_add(pixel_index, sd_float_to_int(sd_vx(tile_coord)));

    return sd_vec4_gather(texture->color, pixel_index);
}

#endif /* TX_BITMAP_H */
