#include <TX/ECS.h>
#include <TX/TX_3D.h>
#include <TX/TX_Bitmap.h>
#include <TX/TX_Resource.h>
#include <TX/Math/linalg.h>

#include "TX_UI_c.h"

/* Printable ASCII characters (0x20-0x7E) */
static constexpr char PRINTABLE_START = 32;
static constexpr char PRINTABLE_COUNT = 95;

void TX_ArrangeText(ECS_Handle *self, vec2 pos, vec2 dim) {
    TX_Text *text = ECS_GetComponent(self, TX_UI.Text);

    if (!text->str)
        return;

    ECS_Handle *world = ECS_GetAncestorWithComponent(self, TX_3D.World, false);
    size_t len = SDL_strlen(text->str);

    if (len > List_Length(text->chars)) {
        size_t nnew = len - List_Length(text->chars);
        TX_TextCharacter *new_chars = List_PushSpace(text->chars, nnew);
        
        for (size_t i = 0; i < nnew; ++i) {
            new_chars[i].geometry = TX_RegisterGeometry(world, text->rect);
            new_chars[i].tile_map = SDL_malloc(sizeof(TX_TileMap));
            new_chars[i].tile_map->texture = text->spritefont;
            new_chars[i].tile_map->tile_width = text->spritefont->width / text->hchars;
            new_chars[i].tile_map->tile_height = text->spritefont->height / text->vchars;
            new_chars[i].tile_map->tile_unit = SDL_max(new_chars[i].tile_map->tile_width, new_chars[i].tile_map->tile_height);

            TX_InstanceWorldGeometry(new_chars[i].geometry,
                (TX_ShaderCall []) { { SD_SELECT(TX_ShadeTileMap), new_chars[i].tile_map }, {}},
                0, TX_RASTERIZER_ALPHA_SCISSOR
            );
        }
    }

    float start = pos.x - dim.x / 2 + (float)text->char_width / 2;

    for (size_t i = 0; i < List_Length(text->chars); ++i) {
        TX_TextCharacter tc = List_Get(text->chars, i);
        char c = SDL_clamp(text->str[i] - PRINTABLE_START, 0, PRINTABLE_COUNT - 1);
        tc.tile_map->x = c % text->hchars;
        tc.tile_map->y = c / text->hchars;

        xform3 transform = i > len ? (xform3) {} : (xform3) { mat3x3_identity, {
            .x = start + i * text->char_width,
            .y = pos.y + dim.y / 2 - (float)text->char_height / 2 
        }};

        TX_SetGeometryTransform(tc.geometry, transform);
    }
}

void TX_SetText(ECS_Handle *self, char *str) {
    TX_Text *text = ECS_GetComponent(self, TX_UI.Text);
    TX_Layout *layout = ECS_GetComponent(self, TX_UI.Layout);
    size_t len = SDL_strlen(str);
    text->str = SDL_realloc(text->str, len + 1);
    SDL_strlcpy(text->str, str, len + 1);
    TX_PropagateLayout(self, layout->pos, layout->dim);
}

void TX_AttachText(ECS_Handle *self, ECS_Component(void) *component) {
    TX_Text *text = ECS_GetComponent(self, component);
    ECS_Handle *tb = ECS_GetAncestorWithComponent(self, TX_Bitmap.TextureBank, false);
    text->spritefont = TX_GetResource(tb, TX_Bitmap.TextureBank, text->spritefont_path);
    text->char_width = text->spritefont->width / text->hchars;
    text->char_height = text->spritefont->height / text->vchars;
    text->rect = TX_CreateRectMesh(text->char_width, text->char_height);

    if (text->str_init)
        TX_SetText(self, text->str_init);
}

void TX_DetachText(ECS_Handle *self, ECS_Component(void) *component) {
    TX_Text *text = ECS_GetComponent(self, component);
    ECS_Handle *tb = ECS_GetAncestorWithComponent(self, TX_Bitmap.TextureBank, false);
    TX_ReleaseResource(tb, TX_Bitmap.TextureBank, text->spritefont_path);
    TX_FreeMesh(text->rect);

    List_ForEach(text->chars, c, {
        TX_FreeWorldGeometry(c.geometry);
        SDL_free(c.tile_map);
    });
}

void TX_InitText(void *component, void *args) {
    TX_Text *text = component;
    TX_TextArgs *text_args = args;
    text->str_init = text_args->str ? SDL_strdup(text_args->str) : nullptr;
    text->str = nullptr;
    text->spritefont_path = SDL_strdup(text_args->spritefont_path);
    text->hchars = text_args->hchars;
    text->vchars = text_args->vchars;
    text->chars = List_Create(TX_TextCharacter);
}

void TX_FreeText(void *component) {
    TX_Text *text = component;
    List_Free(text->chars);
    SDL_free(text->spritefont_path);
    SDL_free(text->str);
    SDL_free(text->str_init);
}
