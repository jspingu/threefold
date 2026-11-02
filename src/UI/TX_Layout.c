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

    ECS_Handle *world = ECS_GetAncestorWithComponent(self, TX_3D.World, false);

    if (world) {
        if (layout->test_geom) {
            TX_FreeWorldGeometry(layout->test_geom);
            TX_FreeMesh(layout->rect);
        }

        layout->rect = TX_CreateRectMesh(dim.x, dim.y);
        layout->test_geom = TX_RegisterGeometry(world, layout->rect);

        TX_SetGeometryTransform(layout->test_geom, (xform3) {
            mat3x3_identity,
            { .x = pos.x, .y = pos.y }
        });

        TX_InstanceWorldGeometry(layout->test_geom,
            (TX_ShaderCall []) { { SD_SELECT(TX_ShadeSolidColor), layout->color } , {}},
            0, 0
        );
    }

    /* Broadcast layout change to matching systems */
    ECS_ProcessEntitySystemGroup(self, TX_SystemGroups.Layout, pos, dim);

    /* Accumulate flow length and grow portions */
    TX_LayoutFlow flow = layout->flow;
    TX_LayoutFlow perp = !flow;
    float flow_whole = 0;
    float flow_length = layout->gap.xy[flow] * (List_Length(layout->cls) - 1);

    List_ForEach(layout->cls, child, {
        TX_Layout *cl = ECS_GetComponent(child, TX_UI.Layout);

        if (cl->grow.xy[flow])
            flow_whole += cl->grow.xy[flow];
        else
            flow_length += cl->dim.xy[flow];
    });

    float flow_rem = SDL_max(dim.xy[flow] - layout->pad.xy[flow] * 2 - flow_length, 0);

    if (flow_whole)
        flow_length += flow_rem;

    /* Select starting position in the flow axis based on alignment */
    float flow_start = pos.xy[flow] + (float [3]) {
        layout->pad.xy[flow] - dim.xy[flow] / 2,               // TX_LAYOUT_ALIGN_START
        -flow_length / 2,                                      // TX_LAYOUT_ALIGN_CENTER
        dim.xy[flow] / 2 - layout->pad.xy[flow] - flow_length  // TX_LAYOUT_ALIGN_END
    }[layout->align[flow]];

    /* If the flow axis is vertical, iterate child layouts in reverse so they are arranged from top to bottom (negative y is down) */
    bool rev = flow == TX_LAYOUT_FLOW_VERTICAL;
    size_t ncls = List_Length(layout->cls);
    
    for (size_t i = rev ? ncls : 0; (rev && i > 0) || (!rev && i < ncls); i += rev ? -1 : 1) {
        ECS_Handle *child = List_Get(layout->cls, i - rev);
        TX_Layout *cl = ECS_GetComponent(child, TX_UI.Layout);
        float perp_length = cl->grow.xy[perp] ? dim.xy[perp] - layout->pad.xy[perp] * 2 : cl->dim.xy[perp];
        vec2 cl_position, cl_dimensions;

        /* Select starting position in the perpendicular axis based on alignment */
        float perp_start = pos.xy[perp] + (float [3]) {
            layout->pad.xy[perp] - dim.xy[perp] / 2,               // TX_LAYOUT_ALIGN_START
            -perp_length / 2,                                      // TX_LAYOUT_ALIGN_CENTER
            dim.xy[perp] / 2 - layout->pad.xy[perp] - perp_length  // TX_LAYOUT_ALIGN_END
        }[layout->align[perp]];

        cl_dimensions.xy[flow] = cl->grow.xy[flow] ? flow_rem * cl->grow.xy[flow] / flow_whole : cl->dim.xy[flow];
        cl_dimensions.xy[perp] = perp_length;
        cl_position.xy[flow] = flow_start + cl_dimensions.xy[flow] / 2;
        cl_position.xy[perp] = perp_start + perp_length / 2;
        flow_start += cl_dimensions.xy[flow] + layout->gap.xy[flow];
        TX_PropagateLayout(child, cl_position, cl_dimensions);
    }
}

void TX_AttachLayout(ECS_Handle *self, ECS_Component(void) *component) {
    ECS_Handle *parent = ECS_GetParent(self);
    TX_Layout *pl = parent ? ECS_GetComponent(parent, component) : nullptr;

    if (pl) {
        List_Push(pl->cls, self);
        TX_PropagateLayout(parent, pl->pos, pl->dim);
    }
    else {
        TX_Layout *layout = ECS_GetComponent(self, component);
        TX_PropagateLayout(self, layout->pos, layout->dim);
    }
}

void TX_DetachLayout(ECS_Handle *self, ECS_Component(void) *component) {
    ECS_Handle *parent = ECS_GetParent(self);
    TX_Layout *pl = parent ? ECS_GetComponent(parent, component) : nullptr;

    if (pl) {
        List_RemoveWhere(pl->cls, child, child == self);
        TX_PropagateLayout(parent, pl->pos, pl->dim);
    }
}

void TX_InitLayout(void *component, void *args) {
    TX_Layout *layout = component;
    SDL_memcpy(layout, args, sizeof(TX_Layout));
    layout->cls = List_Create(ECS_Handle *);

    layout->color = SDL_malloc(sizeof(TX_SolidColor));
    layout->color->r = 0.8;
    layout->color->g = 0.2;
    layout->color->b = 0.7;
}

void TX_FreeLayout(void *component) {
    TX_Layout *layout = component;
    List_Free(layout->cls);
    SDL_free(layout->color);

    if (layout->test_geom) {
        TX_FreeWorldGeometry(layout->test_geom);
        TX_FreeMesh(layout->rect);
    }
}
