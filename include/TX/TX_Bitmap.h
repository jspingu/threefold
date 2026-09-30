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
     * This cubemap sampling algorithm is overly complicated and really difficult to understand. I recall using trial and error to get it to work.
     * Needs a rewrite and documentation. Most of the complexity can be eliminated by preprocessing the cubemap texture into a specific format.
     *
     * There are two issues:
     * - Conversion from view-space coordinates to texture-space coordinates is not trivial
     *     - Consider when the direction vector lands on a cube face aligned with the xy plane
     *     - For a positive z component, this means we should sample the front face
     *     - For a negative z component, this means we should sample the back face
     *     - Naively sampling the corresponding face using the xy coordinates of the intersection point gives incorrect results for multiple reasons
     *     - View-space +y is up, while texture-space +y is down, so we need to flip the y coordinate
     *     - Opposite faces have flipped view-space to texture-space mappings, so we need to flip the x coordinate for the back face
     *         - For the xy and zy faces, it's flipped horizontally
     *         - For the xz faces, it's flipped vertically
     * - Vectorization
     *     - Each stride needs to multiplex 6 texture indices for each face
     *     - With the current approach this requires a lot of masking and blending operations
     *     - This is called in fragment shaders, so we should really be cutting down on these for performance
     */

    /*
     * New approach:
     * - Compute intersection point with the +x, +y, and +z faces of the cube as before, to avoid an abs()
     * - Sample the cube faces directly, the flipping will be preprocessed in the cubemap texture
     *
     * I worked out these VS -> TS mappings
     *
     * Right
     * z, y -> -z, -y
     * Left
     * z, y -> -z, y
     * Up
     * x, z -> x, z
     * Down
     * x, z -> -x, z
     * Front
     * x, y -> x, -y
     * Back
     * x, y -> x, y
     *
     * Texture layout:
     * 
     * +------------+------------+
     * |            |            |
     * |   right    |    left    |
     * |  (-x, -y)  |  (-x, y)   |
     * |            |            |
     * +------------+------------+
     * |            |            |
     * |     up     |    down    |
     * |   (x, y)   |  (-x, y)   |
     * |            |            |
     * +------------+------------+
     * |            |            |
     * |   front    |    back    |
     * |  (x, -y)   |   (x, y)   |
     * |            |            |
     * +------------+------------+
     */


    sd_float unit = sd_float_set(texture->width * 0.5f);
    sd_vec3 rcp = sd_vec3_rcp(dir); // This is not wrapped in an sd_vec3_abs(), so we're always computing intersections in the +x, +y, and +z faces
                                    // The cryptic negations that follow are compensating for this

    sd_vec2 zy = sd_vec2_muls(sd_vec2_create(sd_vz(dir), sd_vy(dir)), sd_float_negate(sd_vx(rcp)));
    sd_vec2 xz = sd_vec2_muls(sd_vec2_create(sd_vx(dir), sd_vz(dir)), sd_float_negate(sd_vy(rcp)));
    sd_vec2 xy = sd_vec2_muls(sd_vec2_create(sd_vx(dir), sd_vy(dir)), sd_vz(rcp));

    sd_mask mask_zy = sd_mask_and(sd_float_between(sd_vx(zy), -1, 1), sd_float_between(sd_vy(zy), -1, 1));
    sd_mask mask_xz = sd_mask_andn(sd_mask_and(sd_float_between(sd_vx(xz), -1, 1), sd_float_between(sd_vy(xz), -1, 1)), mask_zy);

    sd_mask across_zy = sd_float_lt(sd_vx(dir), sd_float_zero());
    sd_mask across_xz = sd_float_lt(sd_vy(dir), sd_float_zero());
    sd_mask across_xy = sd_float_lt(sd_vz(dir), sd_float_zero());

    sd_int idx_zy = sd_int_mask_blend(sd_int_set(0), sd_int_set(3), across_zy);
    sd_int idx_xz = sd_int_mask_blend(sd_int_set(1), sd_int_set(4), across_xz);
    sd_int idx_xy = sd_int_mask_blend(sd_int_set(2), sd_int_set(5), across_xy);

    sd_vec2 flip = sd_vec2_create(
        sd_float_mask_blend(sd_float_one(), sd_float_set(-1), sd_mask_andn(mask_xz, across_xz)),
        sd_float_mask_blend(sd_float_set(-1), sd_float_one(), sd_mask_or(
            sd_mask_andn(mask_zy, across_zy),
            sd_mask_andn(across_xy, sd_mask_or(mask_zy, mask_xz))
        ))
    );

    sd_vec2 pixel_coord = sd_vec2_mask_blend(sd_vec2_mask_blend(xy, xz, mask_xz), zy, mask_zy);
            pixel_coord = sd_vec2_mul(pixel_coord, flip);
            pixel_coord = sd_vec2_adds(sd_vec2_muls(pixel_coord, unit), unit);
            pixel_coord = sd_vec2_clamp(pixel_coord, sd_float_zero(), sd_float_set(texture->width - 1));

    sd_int pixel_offset = sd_int_mask_blend(sd_int_mask_blend(idx_xy, idx_xz, mask_xz), idx_zy, mask_zy);
           pixel_offset = sd_int_mul(pixel_offset, sd_int_set(texture->width * texture->width));

    sd_int pixel_index = sd_int_add(pixel_offset, sd_int_add(sd_int_mul(
        sd_float_to_int(sd_vy(pixel_coord)),
        sd_int_set(texture->width)
    ), sd_float_to_int(sd_vx(pixel_coord))));

    return sd_vec4_gather(texture->color, pixel_index);
}

#endif /* TX_BITMAP_H */
