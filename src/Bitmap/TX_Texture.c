#include <SDL3/SDL.h>
#include <TX/TX_ECS.h>
#include <TX/TX_Resource.h>
#include <TX/gamma.h>

static TX_Texture *TX_TextureFromSDLSurface(SDL_Surface *surface) {
    SDL_Surface *img_abgr = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_ABGR32);

    TX_Texture *texture = SDL_malloc(sizeof(TX_Texture));
    texture->color = SDL_malloc(sizeof(float [4]) * img_abgr->w * img_abgr->h);
    texture->width = img_abgr->w;
    texture->height = img_abgr->h;
    texture->unit = SDL_max(img_abgr->w, img_abgr->h);

    for (int i = 0; i < img_abgr->w * img_abgr->h; ++i) {
        uint32_t *px = img_abgr->pixels;
        texture->color[i * 4 + 0] = (float)gamma_decode_lut[(px[i] >> 24) & 0xFF] / 0xFFFF;
        texture->color[i * 4 + 1] = (float)gamma_decode_lut[(px[i] >> 16) & 0xFF] / 0xFFFF;
        texture->color[i * 4 + 2] = (float)gamma_decode_lut[(px[i] >>  8) & 0xFF] / 0xFFFF;
        texture->color[i * 4 + 3] = (float)(px[i] & 0xFF) / 0xFF;
    }

    SDL_DestroySurface(img_abgr);
    return texture;
}

static void *TX_LoadTextureResource(ECS_Handle *self, char *path) {
    (void)self;
    SDL_Surface *img = SDL_LoadPNG(path);
    TX_Texture *texture = TX_TextureFromSDLSurface(img);
    SDL_DestroySurface(img);
    return texture;
}

static void *TX_LoadCubemapResource(ECS_Handle *self, char *path) {
    (void)self;
    const size_t base_len = sizeof("/posx.png");
    const char *img_suffixes[] = { "/posx.png", "/posy.png", "/posz.png", "/negx.png", "/negy.png", "/negz.png" };
    const SDL_FlipMode flip[] = { SDL_FLIP_HORIZONTAL_AND_VERTICAL, SDL_FLIP_NONE, SDL_FLIP_VERTICAL, SDL_FLIP_HORIZONTAL, SDL_FLIP_HORIZONTAL, SDL_FLIP_NONE };

    SDL_Surface *imgs[SDL_arraysize(img_suffixes)];
    size_t dir_len = SDL_strlen(path);
    char *img_path = SDL_realloc(SDL_strdup(path), dir_len + base_len + 1);

    for (size_t i = 0; i < SDL_arraysize(imgs); ++i) {
        SDL_strlcpy(img_path + dir_len, img_suffixes[i], base_len + 1);
        imgs[i] = SDL_LoadPNG(img_path);
        SDL_FlipSurface(imgs[i], flip[i]);
    }

    SDL_Surface *surface = SDL_CreateSurface(imgs[0]->w * 3, imgs[0]->h * 2, SDL_PIXELFORMAT_ABGR32);

    for (size_t i = 0; i < SDL_arraysize(imgs); ++i) {
        SDL_BlitSurface(imgs[i], nullptr, surface, &(SDL_Rect){
            .x = (i % 3) * imgs[i]->w,
            .y = (i / 3) * imgs[i]->h
        });

        SDL_DestroySurface(imgs[i]);
    }

    TX_Texture *texture = TX_TextureFromSDLSurface(surface);
    texture->unit = texture->width / 6;
    SDL_DestroySurface(surface);
    SDL_free(img_path);
    return texture;
}

static void TX_FreeTextureResource(ECS_Handle *self, void *data) {
    (void)self;
    TX_Texture *texture = data;
    SDL_free(texture->color);
    SDL_free(texture);
}

void TX_AttachTextureBank(ECS_Handle *self, ECS_Component(void) *component) {
    TX_AttachResourceBank(self, component, TX_LoadTextureResource, TX_FreeTextureResource);
}

void TX_AttachCubemapBank(ECS_Handle *self, ECS_Component(void) *component) {
    TX_AttachResourceBank(self, component, TX_LoadCubemapResource, TX_FreeTextureResource);
}
