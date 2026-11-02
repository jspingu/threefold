#ifndef TX_UI_H
#define TX_UI_H

#include <SDL3/SDL.h>
#include <TX/ECS.h>
#include <TX/TX_3D.h>
#include <TX/TX_Bitmap.h>
#include <TX/Collections/List.h>
#include <TX/Math/linalg.h>

typedef enum TX_LayoutFlow {
    TX_LAYOUT_FLOW_HORIZONTAL,
    TX_LAYOUT_FLOW_VERTICAL
} TX_LayoutFlow;

typedef enum TX_LayoutAlign {
    TX_LAYOUT_ALIGN_START,
    TX_LAYOUT_ALIGN_CENTER,
    TX_LAYOUT_ALIGN_END
} TX_LayoutAlign;

typedef struct TX_InputState TX_InputState;

typedef struct TX_Layout {
    List(ECS_Handle *) *cls;
    vec2 pos;
    vec2 dim, grow;
    vec2 pad, gap;
    TX_LayoutFlow flow;
    TX_LayoutAlign align[2];

    TX_Mesh *rect;
    TX_WorldGeometry *test_geom;
    TX_SolidColor *color;
} TX_Layout;

// typedef struct TX_Text {
//     char *str;
//     char *font_path;
//     TX_Texture *font_texture;
//     List(TX_RenderInstance *) *instances;
// } TX_Text;

struct TX_UI {
    ECS_Component(TX_InputState) *InputState;
    ECS_Component(TX_Layout) *Layout;
    // ECS_Component(TX_Text) *Text;
};

extern struct TX_UI TX_UI;

bool TX_IsKeyDown(ECS_Handle *self, SDL_Scancode sc);
bool TX_IsKeyJustDown(ECS_Handle *self, SDL_Scancode sc);
bool TX_IsKeyJustUp(ECS_Handle *self, SDL_Scancode sc);

vec2 TX_GetMouseMotion(ECS_Handle *self);
vec2 TX_GetWheelMotion(ECS_Handle *self);

#endif /* TX_UI_H */
