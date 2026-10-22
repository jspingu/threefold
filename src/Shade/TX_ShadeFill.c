#include <TX/ECS.h>
#include <TX/TX_Shade.h>
#include <TX/TX_Bitmap.h>
#include <TX/Math/stride.h>

SD_CALL sd_vec4 SD_VARIANT(TX_ShadeSolidColor)(void *state, sd_vec4 col, TX_ShaderParams *fragment) {
    (void)col, (void)fragment;
    TX_SolidColor *solid_color = state;
    return sd_vec4_set(solid_color->r, solid_color->g, solid_color->b, 1);
}

SD_CALL sd_vec4 SD_VARIANT(TX_ShadeCheckerboard)(void *state, sd_vec4 col, TX_ShaderParams *fragment) {
    (void)col;
    TX_Checkerboard *checkerboard = state;
    sd_vec2 tile_coord = sd_vec2_muls(*fragment->ts, sd_float_set(checkerboard->tiles));
    sd_int tile_idx = sd_int_add(sd_int_mul(sd_float_to_int(sd_vy(tile_coord)), sd_int_set(checkerboard->tiles)), sd_float_to_int(sd_vx(tile_coord)));
    sd_mask tile_mask = sd_int_gt(sd_int_and(tile_idx, sd_int_set(1)), sd_int_set(0));

    return sd_vec4_mask_blend(
        sd_vec4_set(checkerboard->r1, checkerboard->g1, checkerboard->b1, 1),
        sd_vec4_set(checkerboard->r2, checkerboard->g2, checkerboard->b2, 1),
        tile_mask
    );
}

SD_CALL sd_vec4 SD_VARIANT(TX_ShadeTextureMap)(void *state, sd_vec4 col, TX_ShaderParams *fragment) {
    (void)col;
    TX_TextureMap *texture_map = state;
    return TX_SampleNearest(texture_map->texture, *fragment->ts) ;
}

#ifndef SD_SRC_VARIANT

void TX_AttachTextureMap(ECS_Handle *self, ECS_Component(void) *component) {
    TX_ShaderCall *shader_component = ECS_GetComponent(self, component);
    TX_TextureMap *texture_map = shader_component->state;
    ECS_Handle *tb = ECS_GetAncestorWithComponent(self, TX_Bitmap.TextureBank, false);
    texture_map->texture = TX_GetResource(tb, TX_Bitmap.TextureBank, texture_map->texture_path);
}

void TX_DetachTextureMap(ECS_Handle *self, ECS_Component(void) *component) {
    TX_ShaderCall *shader_component = ECS_GetComponent(self, component);
    TX_TextureMap *texture_map = shader_component->state;
    ECS_Handle *tb = ECS_GetAncestorWithComponent(self, TX_Bitmap.TextureBank, false);
    TX_ReleaseResource(tb, TX_Bitmap.TextureBank, texture_map->texture_path);
}

void TX_InitSolidColor(void *component, void *args) {
    TX_ShaderCall *shader_component = component;
    shader_component->callback = SD_SELECT(TX_ShadeSolidColor);
    shader_component->state = SDL_malloc(sizeof(TX_SolidColor));
    SDL_memcpy(shader_component->state, args, sizeof(TX_SolidColor));
}

void TX_InitCheckerboard(void *component, void *args) {
    TX_ShaderCall *shader_component = component;
    shader_component->callback = SD_SELECT(TX_ShadeCheckerboard);
    shader_component->state = SDL_malloc(sizeof(TX_Checkerboard));
    SDL_memcpy(shader_component->state, args, sizeof(TX_Checkerboard));
}

void TX_InitTextureMap(void *component, void *args) {
    TX_ShaderCall *shader_component = component;
    shader_component->callback = SD_SELECT(TX_ShadeTextureMap);
    shader_component->state = SDL_malloc(sizeof(TX_TextureMap));

    TX_TextureMap *texture_map = shader_component->state;
    char *path = args;
    texture_map->texture_path = SDL_malloc(SDL_strlen(path) + 1);
    SDL_strlcpy(texture_map->texture_path, path, TX_RESOURCE_PATHLEN);
}

void TX_FreeTextureMap(void *component) {
    TX_ShaderCall *shader_component = component;
    TX_TextureMap *texture_map = shader_component->state;
    SDL_free(texture_map->texture_path);
    SDL_free(texture_map);
}

#endif /* SD_SRC_VARIANT */
