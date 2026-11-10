#ifndef TX_BITMAP_H
#define TX_BITMAP_H

#include <SDL3/SDL.h>
#include <TX/ECS.h>
#include <TX/TX_Resource.h>
#include <TX/Collections/List.h>
#include <TX/Math/stride.h>

typedef struct TX_CanvasThreadPool TX_CanvasThreadPool;

typedef struct TX_Window {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    int width, height;
} TX_Window;

typedef struct TX_WindowArgs {
    char *title;
    int width, height;
} TX_WindowArgs;

typedef struct TX_CanvasTile {
    List(size_t) *triangle_idxs;
    sd_int *scanlines[2];
    int left, right;
    int top, bottom;
} TX_CanvasTile;

typedef struct TX_Canvas {
    TX_CanvasThreadPool *thread_pool;
    sd_vec3 *color;
    sd_float *depth;
    TX_CanvasTile *tiles;
    int ntiles, tile_width;
    int width, height;
} TX_Canvas;

typedef struct TX_Texture {
    float *color;
    int width, height;
    int unit;
} TX_Texture;

struct TX_Bitmap {
    ECS_Component(TX_Window) *Window;
    ECS_Component(TX_Canvas) *Canvas;
    ECS_Component(TX_ResourceBank(TX_Texture *)) *TextureBank;
    ECS_Component(TX_ResourceBank(TX_Texture *)) *CubemapBank;
};

extern struct TX_Bitmap TX_Bitmap;

static inline sd_vec4 TX_SampleNearest(TX_Texture *texture, sd_vec2 uts) {
    uts = sd_vec2_clamp(uts, sd_vec2_zero(), sd_vec2_set(texture->width - 1, texture->height - 1));
    sd_int idx = sd_int_mla(sd_float_toi(sd_vx(uts)), sd_float_toi(sd_vy(uts)), sd_int_set(texture->width));
    return sd_vec4_gather(texture->color, idx);
}

static inline sd_vec4 TX_SampleBilinear(TX_Texture *texture, sd_vec2 uts, sd_vec3 bg) {
    sd_int pitch = sd_int_set(texture->width);
    sd_vec2 dm1 = sd_vec2_set(texture->width - 1, texture->height - 1);
    sd_vec2 tl = sd_vec2_clamp(sd_vec2_subs(uts, sd_float_set(0.5)), sd_vec2_zero(), dm1);
    sd_vec2 br = sd_vec2_min(sd_vec2_adds(tl, sd_float_one()), dm1);
    sd_int tlx = sd_float_toi(sd_vx(tl)), tly = sd_float_toi(sd_vy(tl));
    sd_int brx = sd_float_toi(sd_vx(br)), bry = sd_float_toi(sd_vy(br));

    sd_vec4 s00 = sd_vec4_gather(texture->color, sd_int_mla(tlx, tly, pitch));
    sd_vec4 s01 = sd_vec4_gather(texture->color, sd_int_mla(brx, tly, pitch));
    sd_vec4 s10 = sd_vec4_gather(texture->color, sd_int_mla(tlx, bry, pitch));
    sd_vec4 s11 = sd_vec4_gather(texture->color, sd_int_mla(brx, bry, pitch));

    sd_vec3 c00 = sd_vec3_blends(bg, sd_vxyz(s00), sd_vw(s00));
    sd_vec3 c01 = sd_vec3_blends(bg, sd_vxyz(s01), sd_vw(s01));
    sd_vec3 c10 = sd_vec3_blends(bg, sd_vxyz(s10), sd_vw(s10));
    sd_vec3 c11 = sd_vec3_blends(bg, sd_vxyz(s11), sd_vw(s11));

    sd_vec2 lerp = sd_vec2_frac(tl);
    sd_vec3 c = sd_vec3_blends(sd_vec3_blends(c00, c01, sd_vx(lerp)), sd_vec3_blends(c10, c11, sd_vx(lerp)), sd_vy(lerp));
    return sd_vec4_create(c, sd_float_one());
}

static inline sd_vec4 TX_SampleCubemap(TX_Texture *texture, sd_vec3 dir) {
    /* 
     * Sample a cubemap texture given a direction vector.
     * Cubemap textures with a suitable layout can be obtained from CubemapBank.
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

    sd_float hunit = sd_float_sel(mask_zy, sd_float_set(0), sd_float_sel(mask_xz, sd_float_set(2), sd_float_set(4)));
    sd_float vunit = sd_float_sel(across, sd_float_set(2), sd_float_set(0));

    sd_vec2 uts = sd_vec2_sel(mask_zy, zy, sd_vec2_sel(mask_xz, xz, xy));
            uts = sd_vec2_adds(uts, sd_float_one());
            uts = sd_vec2_add(uts, sd_vec2_create(hunit, vunit));
            uts = sd_vec2_muls(uts, sd_float_set(texture->unit));

    return TX_SampleNearest(texture, uts);
}

#endif /* TX_BITMAP_H */
