#include <TX/ECS.h>
#include <TX/TX_ECS.h>
#include <TX/TX_3D.h>
#include <TX/Math/linalg.h>
#include <TX/Math/stride.h>

sd_vec2 SD_VARIANT(TX_ProjectParallel)(ECS_Handle *self, sd_vec3 pos, sd_vec2 midpoint) {
    TX_ParallelProjector *parallel_projector = ECS_GetComponent(self, TX_3D.ParallelProjector);
    sd_vec2 projected = sd_vec2_fsmadd(sd_vec2_set(parallel_projector->slope.x, parallel_projector->slope.y), sd_vz(pos), sd_vxy(pos));
    return sd_vec2_add(midpoint, sd_vec2_mul(projected, sd_vec2_set(parallel_projector->scale.x, -parallel_projector->scale.y)));
}

sd_vec2 SD_VARIANT(TX_ProjectPerspective)(ECS_Handle *self, sd_vec3 pos, sd_vec2 midpoint) {
    TX_PerspectiveFOV *perspective_fov = ECS_GetComponent(self, TX_3D.PerspectiveFOV);

    sd_vec2 normalized = sd_vec2_create(sd_vx(pos), sd_float_negate(sd_vy(pos)));
            normalized = sd_vec2_muls(normalized, sd_float_rcp(sd_float_mul(sd_vz(pos), sd_float_set(perspective_fov->tan_half_fov))));

    return sd_vec2_fsmadd(normalized, sd_vx(midpoint), midpoint);
}

#ifndef SD_SRC_VARIANT

xform3 TX_GetEntityTransform(ECS_Handle *self) {
    mat3x3 *basis = ECS_GetComponent(self, TX_3D.Basis);
    vec3 *position = ECS_GetComponent(self, TX_3D.Position);

    return (xform3) {
        basis ? *basis : mat3x3_identity,
        position ? *position : vec3_zero
    };
}

void TX_TransformEntity(ECS_Handle *self, xform3 lhs) {
    TX_TransformCompositor *compose = ECS_GetComponent(self, TX_3D.TransformCompositor);
    if (!compose) return;

    xform3 composed = (*compose)(self, lhs);
    ECS_ProcessEntitySystemGroup(self, TX_SystemGroups.Transform, composed);
    ECS_ForEachChild(self, c, TX_TransformEntity(c, composed); );
}

xform3 TX_ComposeTransformDefault(ECS_Handle *self, xform3 lhs) {
    return xform3_apply(lhs, TX_GetEntityTransform(self));
}

xform3 TX_ComposeTransformBillboard(ECS_Handle *self, xform3 lhs) {
    xform3 local = TX_GetEntityTransform(self);

    return (xform3) {
        local.basis,
        vec3_add(lhs.translation, mat3x3_mul(lhs.basis, local.translation))
    };
}

xform3 TX_ComposeTransformCubemap(ECS_Handle *self, xform3 lhs) {
    xform3 local = TX_GetEntityTransform(self);

    return (xform3) {
        mat3x3_mul(lhs.basis, local.basis),
        local.translation
    };
}

xform3 TX_ComposeTransformAbsolute(ECS_Handle *self, xform3 lhs) {
    (void)lhs;
    return TX_GetEntityTransform(self);
}

#endif /* SD_SRC_VARIANT */
