#include <TX/ECS.h>
#include <TX/TX_Shade.h>
#include <TX/Collections/List.h>
#include <TX/Math/linalg.h>
#include <TX/Math/stride.h>

SD_CALL sd_vec4 SD_VARIANT(TX_ShadeLighting)(void *state, sd_vec4 col, TX_ShaderParams *fragment) {
    TX_OpticalMedium *medium = state;
    sd_float specularity = sd_float_set(medium->specularity);
    sd_float reflectivity = sd_float_set(medium->reflectivity);
    sd_float ambient = sd_float_set(medium->environment->ambient);
    sd_vec3 eye = sd_vec3_negate(sd_vec3_normalize(*fragment->vs));

    sd_vec3 out = sd_vec3_muls(sd_vxyz(col), ambient);

    List_ForEach(medium->environment->lights, light, {
        sd_vec3 light_vs = sd_vec3_set(light->pos.x, light->pos.y, light->pos.z);
        sd_vec3 light_col = sd_vec3_set(light->col.x, light->col.y, light->col.z);
        sd_float light_energy = sd_float_set(light->energy);

        sd_vec3 incident = sd_vec3_sub(*fragment->vs, light_vs);
        sd_float sqrlen = sd_vec3_dot(incident, incident);
        sd_float rcpsql = sd_float_rcp(sqrlen);
        sd_float rcplen = sd_float_rsqrt(sqrlen);

        sd_vec3 reflected = sd_vec3_muls(sd_vec3_reflect(incident, *fragment->nrml), rcplen);
        sd_float dp = sd_float_max(sd_vec3_dot(reflected, *fragment->nrml), sd_float_zero());

        sd_float rf_falloff = sd_float_sub(sd_float_one(), dp);
                 rf_falloff = sd_float_mul(rf_falloff, rf_falloff);
                 rf_falloff = sd_float_mul(rf_falloff, rf_falloff);

        sd_float rf_coeff = sd_float_fmadd(sd_float_sub(sd_float_one(), reflectivity), rf_falloff, reflectivity);
        sd_float sp_coeff = sd_vec3_dot(eye, reflected);

        for (int i = 0; i < medium->exp; ++i)
            sp_coeff = sd_float_mul(sp_coeff, sp_coeff);

        sp_coeff = sd_float_mul(sp_coeff, specularity);

        sd_vec3 power_in = sd_vec3_muls(light_col, sd_float_mul(sd_float_mul(light_energy, rcpsql), dp));
        sd_vec3 power_out = sd_vec3_muls(power_in, rf_coeff);
        out = sd_vec3_add(out, sd_vec3_mul(sd_vxyz(col), sd_vec3_fsmadd(power_out, sp_coeff, power_out)));
    });

    sd_vec3 dir_vs = sd_vec3_reflect(sd_vec3_normalize(*fragment->vs), *fragment->nrml);
    sd_float dp = sd_vec3_dot(dir_vs, *fragment->nrml);

    sd_float rf_falloff = sd_float_sub(sd_float_one(), dp);
             rf_falloff = sd_float_mul(rf_falloff, rf_falloff);
             rf_falloff = sd_float_mul(rf_falloff, rf_falloff);

    sd_float rf_coeff = sd_float_fmadd(sd_float_sub(sd_float_one(), reflectivity), rf_falloff, reflectivity);

    sd_vec3 dir = sd_vec3_muls(*fragment->vs2ws_xform[0], sd_vx(dir_vs));
            dir = sd_vec3_fsmadd(*fragment->vs2ws_xform[1], sd_vy(dir_vs), dir);
            dir = sd_vec3_fsmadd(*fragment->vs2ws_xform[2], sd_vz(dir_vs), dir);

    sd_vec4 sky = TX_SampleCubemap(medium->environment->sky, dir);
    sd_vec3 power_in = sd_vec3_muls(sd_vxyz(sky), dp);
    sd_vec3 power_out = sd_vec3_muls(power_in, rf_coeff);

    out = sd_vec3_fsmadd(sd_vec3_mul(sd_vxyz(col), power_out), specularity, out);
    return sd_vec4_create(sd_vx(out), sd_vy(out), sd_vz(out), sd_vw(col));
}

SD_CALL sd_vec4 SD_VARIANT(TX_ShadeSky)(void *state, sd_vec4 col, TX_ShaderParams *fragment) {
    (void)col;
    TX_LightEnvironment **env = state;

    sd_vec3 dir = sd_vec3_muls(*fragment->vs2ws_xform[0], sd_vx(*fragment->vs));
            dir = sd_vec3_fsmadd(*fragment->vs2ws_xform[1], sd_vy(*fragment->vs), dir);
            dir = sd_vec3_fsmadd(*fragment->vs2ws_xform[2], sd_vz(*fragment->vs), dir);

    return TX_SampleCubemap((*env)->sky, dir);
}

#ifndef SD_SRC_VARIANT

void TX_AttachLightEnvironment(ECS_Handle *self, ECS_Component(void) *component) {
    TX_LightEnvironment **env = ECS_GetComponent(self, component);
    ECS_Handle *tb = ECS_GetAncestorWithComponent(self, TX_Bitmap.CubemapBank, false);
    (*env)->sky = TX_GetResource(tb, TX_Bitmap.CubemapBank, (*env)->skybox_dir);
}

void TX_DetachLightEnvironment(ECS_Handle *self, ECS_Component(void) *component) {
    TX_LightEnvironment **env = ECS_GetComponent(self, component);
    ECS_Handle *tb = ECS_GetAncestorWithComponent(self, TX_Bitmap.TextureBank, false);
    TX_ReleaseResource(tb, TX_Bitmap.TextureBank, (*env)->skybox_dir);
}

void TX_InitLightEnvironment(void *component, void *args) {
    TX_LightEnvironment **env = component;
    TX_LightEnvironment *env_args = args;
    *env = SDL_malloc(sizeof(TX_LightEnvironment));
    (*env)->skybox_dir = SDL_malloc(SDL_strlen(env_args->skybox_dir) + 1);
    SDL_strlcpy((*env)->skybox_dir, env_args->skybox_dir, TX_RESOURCE_PATHLEN);
    (*env)->lights = List_Create(TX_ActiveLight *);
    (*env)->ambient = env_args->ambient;
}

void TX_FreeLightEnvironment(void *component) {
    TX_LightEnvironment **env = component;
    SDL_free((*env)->skybox_dir);
    List_Free((*env)->lights);
    SDL_free(*env);
}

void TX_TransformPointLight(ECS_Handle *self, xform3 composed) {
    TX_PointLight *light = ECS_GetComponent(self, TX_Shade.PointLight);
    light->active->pos = composed.translation;
}

void TX_AttachPointLight(ECS_Handle *self, ECS_Component(void) *component) {
    TX_PointLight *light = ECS_GetComponent(self, component);
    ECS_Handle *env = ECS_GetAncestorWithComponent(self, TX_Shade.LightEnvironment, false);
    light->environment = *ECS_GetComponent(env, TX_Shade.LightEnvironment);

    TX_ActiveLight *active = SDL_malloc(sizeof(TX_ActiveLight));
    active->col = light->col;
    active->energy = light->energy;
    active->pos = vec3_zero;

    light->active = active;
    List_Push(light->environment->lights, active);
}

void TX_DetachPointLight(ECS_Handle *self, ECS_Component(void) *component) {
    TX_PointLight *light = ECS_GetComponent(self, component);
    List_RemoveWhere(light->environment->lights, active, active == light->active);
    SDL_free(light->active);
}

void TX_AttachLighting(ECS_Handle *self, ECS_Component(void) *component) {
    TX_ShaderCall *shader_component = ECS_GetComponent(self, component);
    TX_OpticalMedium *medium = shader_component->state;
    ECS_Handle *env = ECS_GetAncestorWithComponent(self, TX_Shade.LightEnvironment, false);
    medium->environment = *ECS_GetComponent(env, TX_Shade.LightEnvironment);
}

void TX_InitLighting(void *component, void *args) {
    TX_ShaderCall *shader_component = component;
    shader_component->callback = SD_SELECT(TX_ShadeLighting);
    shader_component->state = SDL_malloc(sizeof(TX_OpticalMedium));
    SDL_memcpy(shader_component->state, args, sizeof(TX_OpticalMedium));
}

void TX_AttachSky(ECS_Handle *self, ECS_Component(void) *component) {
    TX_ShaderCall *shader_component = ECS_GetComponent(self, component);
    ECS_Handle *env = ECS_GetAncestorWithComponent(self, TX_Shade.LightEnvironment, false);
    *(TX_LightEnvironment **)(shader_component->state) = *ECS_GetComponent(env, TX_Shade.LightEnvironment);
}

void TX_InitSky(void *component, void *args) {
    (void)args;
    TX_ShaderCall *shader_component = component;
    shader_component->callback = SD_SELECT(TX_ShadeSky);
    shader_component->state = SDL_malloc(sizeof(TX_LightEnvironment **));
}

#endif
