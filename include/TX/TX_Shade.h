#ifndef TX_SHADE_H
#define TX_SHADE_H

#include <TX/ECS.h>
#include <TX/TX_Bitmap.h>
#include <TX/Math/linalg.h>
#include <TX/Math/stride.h>

#define TX_SHADER_DECLARE(name)  SD_DECLARE_ATTR(SD_CALL, sd_vec4, name, void *, state, sd_vec4, col, TX_ShaderParams *, fragment)

typedef struct TX_ShaderParams TX_ShaderParams;
typedef SD_CALL sd_vec4 (*TX_FragmentShader)(void *state, sd_vec4 col, TX_ShaderParams *fragment);

typedef struct TX_ShaderParams {
    sd_vec3 *bg;
    sd_vec3 *vs;
    sd_vec3 *nrml;
    sd_vec2 *ts;
    sd_vec3 *vs2ws_xform[3];
} TX_ShaderParams;

typedef struct TX_ShaderCall {
    TX_FragmentShader callback;
    void *state;
} TX_ShaderCall;

typedef struct TX_ActiveLight {
    float energy;
    vec3 col;
    vec3 pos;
} TX_ActiveLight;

typedef struct TX_LightEnvironment {
    char *skybox_dir;
    TX_Texture *sky;
    List(TX_ActiveLight *) *lights;
    float ambient;
} TX_LightEnvironment;

typedef struct TX_PointLight {
    TX_LightEnvironment *environment;
    TX_ActiveLight *active;
    vec3 col;
    float energy;
} TX_PointLight;

typedef struct TX_OpticalMedium {
    TX_LightEnvironment *environment;
    float reflectivity;
    float specularity;
    int exp;
} TX_OpticalMedium;

typedef struct TX_SolidColor {
    float r, g, b;
} TX_SolidColor;

typedef struct TX_Checkerboard {
    int tiles;
    float r1, g1, b1;
    float r2, g2, b2;
} TX_Checkerboard;

typedef struct TX_TextureMap {
    TX_Texture *texture;
    char *texture_path;
    float scale;
} TX_TextureMap;

typedef struct TX_TileMap {
    TX_Texture *texture;
    char *texture_path;
    float tile_width, tile_height;
    float tile_unit;
    int htiles, vtiles;
    int x, y;
} TX_TileMap;

struct TX_Shade {
    ECS_Component(TX_LightEnvironment *) *LightEnvironment;
    ECS_Component(TX_PointLight) *PointLight;

    ECS_Component(TX_ShaderCall) *SolidColor;
    ECS_Component(TX_ShaderCall) *Checkerboard;
    ECS_Component(TX_ShaderCall) *TextureMap;
    ECS_Component(TX_ShaderCall) *TileMap;
    ECS_Component(TX_ShaderCall) *Lighting;
    ECS_Component(TX_ShaderCall) *Sky;
};

extern struct TX_Shade TX_Shade;

TX_SHADER_DECLARE(TX_ShadeSolidColor)
TX_SHADER_DECLARE(TX_ShadeCheckerboard)
TX_SHADER_DECLARE(TX_ShadeTextureMap)
TX_SHADER_DECLARE(TX_ShadeTileMap)
TX_SHADER_DECLARE(TX_ShadeLighting)
TX_SHADER_DECLARE(TX_ShadeSky)

#endif /* TX_SHADE_H */
