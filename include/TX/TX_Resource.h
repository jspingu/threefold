#ifndef TX_RESOURCE_H
#define TX_RESOURCE_H

#include <TX/ECS.h>
#include <TX/Collections/Strmap.h>

#define TX_ResourceBank(type)                type
#define TX_GetResource(self,component,path)  ( (typeof(*component))TX_GetResourceActual(self, component, path) )

#define TX_RESOURCE_PATHLEN  64

typedef void *(*TX_ResourceLoader)(ECS_Handle *self, char *path);
typedef void (*TX_ResourceFreer)(ECS_Handle *self, void *data);

typedef struct TX_Resource {
    void *data;
    size_t refcount;
} TX_Resource;

typedef struct TX_ResourceBank {
    Strmap(TX_Resource) *map;
    TX_ResourceLoader load;
    TX_ResourceFreer free;
} TX_ResourceBank;

void *TX_GetResourceActual(ECS_Handle *self, ECS_Component(void) *component, char *path);
void TX_ReleaseResource(ECS_Handle *self, ECS_Component(void) *component, char *path);
void TX_AttachResourceBank(ECS_Handle *self, ECS_Component(void) *component, TX_ResourceLoader load, TX_ResourceFreer free);
void TX_DetachResourceBank(ECS_Handle *self, ECS_Component(void) *component);

#endif /* TX_RESOURCE_H */
