#ifndef ECS_H
#define ECS_H

#include <SDL3/SDL_stdinc.h>

#define ECS_Component(type)                      typeof(type)
#define ECS_SystemGroup(callback_type)           typeof(callback_type)

#define ECS_GetComponent(e,component)            ( (typeof(component))ECS_GetComponentActual(e, component) )
#define ECS_AttachComponents(e,...)              ( ECS_AttachComponentsCount(e, (ECS_ComponentConstruction []){__VA_ARGS__}, SDL_arraysize(((ECS_ComponentConstruction []){__VA_ARGS__}))) )
#define ECS_DetachComponents(e,...)              ( ECS_DetachComponentsCount(e, (void *[]){__VA_ARGS__}, SDL_arraysize(((void *[]){__VA_ARGS__}))) )
#define ECS_AddChildren(e,...)                   ( ECS_AddChildrenCount(e, (ECS_Construction []){__VA_ARGS__}, SDL_arraysize(((ECS_Construction []){__VA_ARGS__}))) )

#define ECS_RegisterComponent(ecs,cmp_type,...)  ( ECS_RegisterComponentActual(ecs, sizeof(cmp_type), (ECS_ComponentCallbacks)__VA_ARGS__) )
#define ECS_RegisterSystem(group,callback,...)   ( *(typeof(group))ECS_RegisterSystemActual(group, (void *[]){__VA_ARGS__}, SDL_arraysize(((void *[]){__VA_ARGS__}))) = callback )

#define ECS_Components(...)                                                                    \
    .ncomponent_constructions = SDL_arraysize(((ECS_ComponentConstruction []){__VA_ARGS__})),  \
    .component_constructions = (ECS_ComponentConstruction []){__VA_ARGS__}

#define ECS_Children(...)                                                                      \
    .nchildren = SDL_arraysize(((ECS_Construction []){__VA_ARGS__})),                          \
    .children = (ECS_Construction []){__VA_ARGS__}

#define ECS_ProcessSystemGroup(group,...)  do {                                               \
    for (size_t i = 0, n = ECS_GetSystemGroupLength(group); i < n; ++i) {                     \
        typeof(*group) callback = (typeof(*group))(ECS_GetSystemGroupCallback(group, i));     \
        ECS *ecs = ECS_GetSystemGroupECS(group);                                              \
        for (ECS_Handle *e = ECS_GetRoot(ecs); e ; e = ECS_NextEntity(e))                     \
            if (ECS_EntityMatchesSystem(e, group, i)) callback(e __VA_OPT__(,) __VA_ARGS__);  \
    }                                                                                         \
} while (0)

#define ECS_ProcessSystemGroupReverse(group,...)  do {                                        \
    for (size_t i = 0, n = ECS_GetSystemGroupLength(group); i < n; ++i) {                     \
        typeof(*group) callback = (typeof(*group))(ECS_GetSystemGroupCallback(group, i));     \
        ECS *ecs = ECS_GetSystemGroupECS(group);                                              \
        for (ECS_Handle *e = ECS_GetLast(ecs); e ; e = ECS_PrevEntity(e))                     \
            if (ECS_EntityMatchesSystem(e, group, i)) callback(e __VA_OPT__(,) __VA_ARGS__);  \
    }                                                                                         \
} while (0)

#define ECS_ProcessEntitySystemGroup(e,group,...)  do {                                    \
    for (size_t i = 0, n = ECS_GetSystemGroupLength(group); i < n; ++i) {                  \
        typeof(*group) callback = (typeof(*group))(ECS_GetSystemGroupCallback(group, i));  \
        if (ECS_EntityMatchesSystem(e, group, i))                                          \
            callback(e __VA_OPT__(,) __VA_ARGS__);                                         \
    }                                                                                      \
} while (0)

#define ECS_ForEachChild(e,child,...)  do {                                                \
    ECS_Handle **ECS_children = ECS_GetChildren(e);                                        \
    size_t ECS_child_count = ECS_GetChildCount(e);                                         \
    for (size_t ECS_child_iter = 0; ECS_child_iter < ECS_child_count; ++ECS_child_iter) {  \
        ECS_Handle *child = ECS_children[ECS_child_iter];                                  \
        do __VA_ARGS__ while (0);                                                          \
    }                                                                                      \
} while (0)

typedef struct ECS ECS;
typedef struct ECS_Handle ECS_Handle;

typedef struct ECS_ComponentCallbacks {
    void (*init)(void *component, void *args);
    void (*free)(void *component);
    void (*attach)(ECS_Handle *self, ECS_Component(void) *component);
    void (*detach)(ECS_Handle *self, ECS_Component(void) *component);
} ECS_ComponentCallbacks;

typedef struct ECS_ComponentConstruction {
    void *component;
    void *structure;
} ECS_ComponentConstruction;

typedef struct ECS_Construction {
    ECS_ComponentConstruction *component_constructions;
    struct ECS_Construction *children;
    size_t ncomponent_constructions;
    size_t nchildren;
} ECS_Construction;

void *ECS_GetComponentActual(ECS_Handle *e, void *component);
ECS_Handle **ECS_GetChildren(ECS_Handle *e);
size_t ECS_GetChildCount(ECS_Handle *e);

ECS_Handle *ECS_GetAncestorWithComponent(ECS_Handle *e, void *component, bool inclusive);
ECS_Handle *ECS_GetDescendantWithComponent(ECS_Handle *e, void *component, bool inclusive);

void ECS_AttachComponentsCount(ECS_Handle *e, ECS_ComponentConstruction *components, size_t count);
void ECS_DetachComponentsCount(ECS_Handle *e, void **components, size_t count);
void ECS_AddChildrenCount(ECS_Handle *e, ECS_Construction *children, size_t count);

ECS_Handle *ECS_NextEntity(ECS_Handle *e);
ECS_Handle *ECS_PrevEntity(ECS_Handle *e);
bool ECS_EntityMatchesSystem(ECS_Handle *e, void *system_group, size_t index);

void ECS_FreeEntity(ECS_Handle *e);

void *ECS_RegisterComponentActual(ECS *ecs, size_t component_size, ECS_ComponentCallbacks callbacks);
void *ECS_RegisterSystemGroup(ECS *ecs);
SDL_FunctionPointer *ECS_RegisterSystemActual(void *system_group, void **dependencies, size_t ndependencies);

ECS *ECS_GetSystemGroupECS(void *system_group);
size_t ECS_GetSystemGroupLength(void *system_group);
SDL_FunctionPointer ECS_GetSystemGroupCallback(void *system_group, size_t system_index);

ECS_Handle *ECS_GetRoot(ECS *ecs);
ECS_Handle *ECS_GetLast(ECS *ecs);

void ECS_Update(ECS *ecs);

ECS *ECS_Create(void);
void ECS_Free(ECS *ecs);

#endif /* ECS_H */
