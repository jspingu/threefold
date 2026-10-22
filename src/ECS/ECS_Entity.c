#include <SDL3/SDL.h>
#include <TX/Collections/List.h>
#include <TX/Collections/Bitset.h>
#include <TX/ECS.h>

#include "ECS_c.h"

static ECS_ComponentConstruction ECS_ConstructComponent(ECS_ComponentConstruction cc) {
    ECS_Handle *component = cc.component;
    ECS_Column column = List_Get(component->ecs->columns, component->index);
    void *structure = SDL_malloc(column.component_size);
    void (*init)(void *, void *) = column.component_callbacks.init;

    if (init)
        init(structure, cc.structure);
    else if (cc.structure)
        SDL_memcpy(structure, cc.structure, column.component_size);

    return (ECS_ComponentConstruction) {
        .component = component,
        .structure = structure
    };
}

static ECS_Construction ECS_PrepareConstruction(ECS_Construction c) {
    size_t ncomponent_constructions = 0;
    size_t nchildren = 0;

    if (c.component_constructions)
        for (ECS_ComponentConstruction *cc = c.component_constructions; cc->component; ++cc)
            ncomponent_constructions += 1;

    if (c.children)
        for (ECS_Construction *child = c.children; child->component_constructions || child->children; ++child)
            nchildren += 1;

    ECS_ComponentConstruction *components = SDL_malloc(sizeof(ECS_ComponentConstruction) * ncomponent_constructions);
    ECS_Construction *children = SDL_malloc(sizeof(ECS_Construction) * nchildren);
    Bitset *seen_components = Bitset_Create();
    size_t nunique_components = 0;

    for (size_t i = 0; i < ncomponent_constructions; ++i) {
        ECS_Handle *component = c.component_constructions[i].component;

        if (!Bitset_Test(seen_components, component->index)) {
            components[nunique_components++] = ECS_ConstructComponent(c.component_constructions[i]);
            Bitset_Set(seen_components, component->index);
        }
    }

    Bitset_Free(seen_components);

    for (size_t i = 0; i < nchildren; ++i)
        children[i] = ECS_PrepareConstruction(c.children[i]);

    return (ECS_Construction) {
        .component_constructions = components,
        .children = children,
        .ncomponent_constructions = nunique_components,
        .nchildren = nchildren
    };
}

static bool ECS_IsComponentAttachmentQueued(ECS_Handle *e, size_t component_index) {
    List_ForEach(e->ecs->headers[e->index].components_attach, to_attach, {
        ECS_Handle *component = to_attach.component;

        if (component->index == component_index)
            return true;
    });

    return false;
}

static bool ECS_IsComponentDetachmentQueued(ECS_Handle *e, size_t component_index) {
    List_ForEach(e->ecs->headers[e->index].components_detach, to_detach, {
        if (to_detach->index == component_index)
            return true;
    });

    return false;
}

void *ECS_GetComponentActual(ECS_Handle *e, void *component) {
    ECS_Handle *c = component;
    ECS_Column column = List_Get(e->ecs->columns, c->index);
    bool has_component = Bitset_Test(e->ecs->headers[e->index].active_components, c->index);
    return has_component ? column.component_entries + column.component_size * e->index : nullptr;
}

ECS_Handle *ECS_GetParent(ECS_Handle *e) {
    return e->ecs->headers[e->index].parent;
}

ECS_Handle **ECS_GetChildren(ECS_Handle *e) {
    return List_GetAddress(e->ecs->headers[e->index].children, 0);
}

size_t ECS_GetChildCount(ECS_Handle *e) {
    return List_Length(e->ecs->headers[e->index].children);
}

ECS_Handle *ECS_GetAncestorWithComponent(ECS_Handle *e, void *component, bool inclusive) {
    for (ECS_Handle *ancestor = inclusive ? e : ECS_GetParent(e); ancestor; ancestor = ECS_GetParent(ancestor)) {
        if (ECS_GetComponentActual(ancestor, component))
            return ancestor;
    }

    return nullptr;
}

ECS_Handle *ECS_GetDescendantWithComponent(ECS_Handle *e, void *component, bool inclusive) {
    for (size_t index = e->index + !inclusive, count = inclusive ? 1 : ECS_GetChildCount(e); count; ++index) {
        ECS_Handle *descendant = e->ecs->headers[index].self;

        if (ECS_GetComponentActual(descendant, component))
            return descendant;

        count += ECS_GetChildCount(descendant) - 1;
    }

    return nullptr;
}

void ECS_AttachComponentsActual(ECS_Handle *e, ECS_ComponentConstruction *ccs) {
    ECS_EntityHeader header = e->ecs->headers[e->index];

    for (ECS_ComponentConstruction *cc = ccs; cc->component; ++cc) {
        ECS_Handle *component = cc->component;

        if (!Bitset_Test(header.active_components, component->index) && !ECS_IsComponentAttachmentQueued(e, component->index))
            List_Push(header.components_attach, ECS_ConstructComponent(*cc));
    }
}

void ECS_DetachComponentsActual(ECS_Handle *e, void **components) {
    ECS_EntityHeader header = e->ecs->headers[e->index];

    for (void **component = components; *component; ++component) {
        ECS_Handle *c = *component;

        if (Bitset_Test(header.active_components, c->index) && !ECS_IsComponentDetachmentQueued(e, c->index))
            List_Push(header.components_detach, c);
    }
    
}

void ECS_AddChildrenActual(ECS_Handle *e, ECS_Construction *children) {
    ECS_EntityHeader header = e->ecs->headers[e->index];

    for (ECS_Construction *c = children; c->component_constructions || c->children; ++c)
        List_Push(header.children_add, ECS_PrepareConstruction(*c));
}

ECS_Handle *ECS_NextEntity(ECS_Handle *e) {
    return e->index < e->ecs->length - 1 ? e->ecs->headers[e->index + 1].self : nullptr;
}

ECS_Handle *ECS_PrevEntity(ECS_Handle *e) {
    return e->index ? e->ecs->headers[e->index - 1].self : nullptr;
}

bool ECS_EntityMatchesSystem(ECS_Handle *e, void *system_group, size_t index) {
    ECS_Handle *group_handle = system_group;
    ECS_SystemGroup group = List_Get(group_handle->ecs->system_groups, group_handle->index);
    ECS_System system = List_Get(group.systems, index);

    for (size_t i = 0; i < system.ndependencies; ++i) {
        if (!Bitset_Test(e->ecs->headers[e->index].active_components, system.dependencies[i]))
            return false;
    }

    return true;
}

void ECS_FreeEntity(ECS_Handle *e) {
    for (size_t index = e->index, count = 1; count; ++index) {
        ECS_EntityHeader *header = e->ecs->headers + index;
        Bitset_ForEach(header->active_components, cindex, ECS_DetachComponents(header->self, List_Get(e->ecs->columns, cindex).component); );
        header->free = true;
        count += ECS_GetChildCount(header->self) - 1;
    }
}
