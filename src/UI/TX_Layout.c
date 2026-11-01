#include <TX/ECS.h>
#include <TX/TX_ECS.h>
#include <TX/TX_3D.h>
#include <TX/TX_Bitmap.h>
#include <TX/TX_Shade.h>
#include <TX/Collections/List.h>
#include <TX/Math/linalg.h>
#include <TX/Math/stride.h>

#include "TX_UI_c.h"

void TX_PropagateLayout(ECS_Handle *self, vec2 pos, vec2 dim) {
    TX_Layout *layout = ECS_GetComponent(self, TX_UI.Layout);
    layout->pos = pos;
    layout->dim = dim;

    /* Draw test rectangle for each layout */
    // static float c = 0.4;
    // ECS_Handle *world = ECS_GetAncestorWithComponent(self, TX_3D.World, false);
    // if (world) {
    //     TX_Mesh *rect = TX_CreateRectMesh(dim.x, dim.y);
    //     TX_WorldGeometry *geom = TX_RegisterGeometry(world, rect);
    //     TX_SetGeometryTransform(geom, (xform3) { mat3x3_identity, (vec3) {
    //         .x = pos.x + dim.x / 2,
    //         .y = pos.y + dim.y / 2,
    //     } });
    //     TX_SolidColor *color = SDL_malloc(sizeof(TX_SolidColor));
    //     *color = (TX_SolidColor) { .r = c, .g = 0.5, .b = 0.5 };
    //     c+=0.1;
    //     TX_RenderInstance *inst = TX_InstanceWorldGeometry(geom,
    //         (TX_ShaderCall []) { { SD_SELECT(TX_ShadeSolidColor), color }, {} },
    //         0, 0
    //     ); 
    // }

    /* Broadcast layout change to matching systems */
    ECS_ProcessEntitySystemGroup(self, TX_SystemGroups.Layout, pos, dim);

    /* Accumulate child layouts, flow length, and grow portions */
    List(ECS_Handle *) *cls = List_Create(ECS_Handle *);
    ECS_Handle **children = ECS_GetChildren(self);
    size_t nchildren = ECS_GetChildCount(self);
    TX_LayoutFlow flow = layout->flow;
    TX_LayoutFlow perp = !flow;
    float flow_whole = 0;
    float flow_length = 0;

    for (size_t i = 0; i < nchildren; ++i) {
        ECS_Handle *child = children[i];
        TX_Layout *cl = ECS_GetComponent(child, TX_UI.Layout);

        if (!cl)
            continue;

        if (List_Length(cls))
            flow_length += layout->gap.xy[flow];

        List_Push(cls, child);

        if (cl->grow.xy[flow])
            flow_whole += cl->grow.xy[flow];
        else
            flow_length += cl->dim.xy[flow];
    }

    float flow_rem = SDL_max(dim.xy[flow] - layout->pad.xy[flow] * 2 - flow_length, 0);

    if (flow_whole)
        flow_length += flow_rem;

    /* Select starting position in the flow axis based on alignment */
    float flow_start = pos.xy[flow] + (float [3]) {
        layout->pad.xy[flow],                              // TX_LAYOUT_ALIGN_START
        (dim.xy[flow] - flow_length) / 2,                  // TX_LAYOUT_ALIGN_CENTER
        dim.xy[flow] - layout->pad.xy[flow] - flow_length  // TX_LAYOUT_ALIGN_END
    }[layout->align[flow]];

    /* If the flow axis is vertical, iterate child layouts in reverse so they are arranged from top to bottom (negative y is down) */
    bool rev = flow == TX_LAYOUT_FLOW_VERTICAL;
    size_t ncls = List_Length(cls);
    
    for (size_t i = rev ? ncls : 0; (rev && i > 0) || (!rev && i < ncls); i += rev ? -1 : 1) {
        ECS_Handle *child = List_Get(cls, i - rev);
        TX_Layout *cl = ECS_GetComponent(child, TX_UI.Layout);
        float perp_length = cl->grow.xy[perp] ? dim.xy[perp] - layout->pad.xy[perp] * 2 : cl->dim.xy[perp];
        vec2 cl_position, cl_dimensions;

        /* Select starting position in the perpendicular axis based on alignment */
        float perp_start = pos.xy[perp] + (float [3]) {
            layout->pad.xy[perp],                              // TX_LAYOUT_ALIGN_START
            (dim.xy[perp] - perp_length) / 2,                  // TX_LAYOUT_ALIGN_CENTER
            dim.xy[perp] - layout->pad.xy[perp] - perp_length  // TX_LAYOUT_ALIGN_END
        }[layout->align[perp]];

        cl_position.xy[flow] = flow_start;
        cl_position.xy[perp] = perp_start;
        cl_dimensions.xy[flow] = cl->grow.xy[flow] ? flow_rem * cl->grow.xy[flow] / flow_whole : cl->dim.xy[flow];
        cl_dimensions.xy[perp] = perp_length;
        flow_start += cl_dimensions.xy[flow] + layout->gap.xy[flow];
        TX_PropagateLayout(child, cl_position, cl_dimensions);
    }

    List_Free(cls);
}

void TX_AttachLayout(ECS_Handle *self, ECS_Component(void) *component) {
    (void)component;
    /* Layout on an entity with a canvas is a root layout */
    TX_Canvas *canvas = ECS_GetComponent(self, TX_Bitmap.Canvas);

    if (canvas)
        TX_PropagateLayout(self,
            (vec2) { .x = -canvas->width / 2.0, .y = -canvas->height / 2.0 },
            (vec2) { .x = canvas->width, .y = canvas->height }
        );
}
