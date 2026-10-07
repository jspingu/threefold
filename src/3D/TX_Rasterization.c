#include <SDL3/SDL.h>
#include <TX/ECS.h>
#include <TX/TX_ECS.h>
#include <TX/Collections/List.h>
#include <TX/Math/linalg.h>
#include <TX/Math/stride.h>

#include "TX_3D_c.h"

static inline vec3 intersect_near(vec3 from, vec3 to, float near) {
    vec3 path = vec3_sub(to, from);
    vec3 slope = vec3_div(path, path.z);
    return vec3_add(from, vec3_mul(slope, near - from.z));
}

static inline void swap(int *a, int *b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

void SD_VARIANT(TX_ScanLinear)(ECS_Handle *self, TX_CanvasTile tile, TX_RasterizerFlags flags, TX_TriangleDraw triangle, int triangle_bounds[2]) {
    TX_Rasterizer *rasterizer = ECS_GetComponent(self, TX_Components.Rasterizer);
    TX_Canvas *canvas = ECS_GetComponent(rasterizer->target, TX_Components.Canvas);
    xform3 scalar_vs2ws_xform = TX_GetEntityTransform(self);

    sd_vec3 vs2ws_xform_i = sd_vec3_set(scalar_vs2ws_xform.basis.x.x, scalar_vs2ws_xform.basis.x.y, scalar_vs2ws_xform.basis.x.z);
    sd_vec3 vs2ws_xform_j = sd_vec3_set(scalar_vs2ws_xform.basis.y.x, scalar_vs2ws_xform.basis.y.y, scalar_vs2ws_xform.basis.y.z);
    sd_vec3 vs2ws_xform_k = sd_vec3_set(scalar_vs2ws_xform.basis.z.x, scalar_vs2ws_xform.basis.z.y, scalar_vs2ws_xform.basis.z.z);

    sd_vec2 origin = sd_vec2_set(triangle.ss_verts[0].x ,triangle.ss_verts[0].y);
    sd_vec2 ab = sd_vec2_sub(sd_vec2_set(triangle.ss_verts[1].x, triangle.ss_verts[1].y), origin);
    sd_vec2 ac = sd_vec2_sub(sd_vec2_set(triangle.ss_verts[2].x, triangle.ss_verts[2].y), origin);

    sd_float inv_disc = sd_float_rcp(sd_float_sub(sd_float_mul(sd_vx(ab), sd_vy(ac)), sd_float_mul(sd_vy(ab), sd_vx(ac))));

    sd_vec2 inv_xform_i = sd_vec2_muls(sd_vec2_create(sd_vy(ac), sd_float_negate(sd_vy(ab))), inv_disc);
    sd_vec2 inv_xform_j = sd_vec2_muls(sd_vec2_create(sd_float_negate(sd_vx(ac)), sd_vx(ab)), inv_disc);

    sd_vec3 origin_vs = sd_vec3_set(triangle.vs_verts[0].x, triangle.vs_verts[0].y, triangle.vs_verts[0].z);
    sd_vec3 ab_vs = sd_vec3_sub(sd_vec3_set(triangle.vs_verts[1].x, triangle.vs_verts[1].y, triangle.vs_verts[1].z), origin_vs);
    sd_vec3 ac_vs = sd_vec3_sub(sd_vec3_set(triangle.vs_verts[2].x, triangle.vs_verts[2].y, triangle.vs_verts[2].z), origin_vs);

    sd_vec3 vs_xform_i = sd_vec3_fsmadd(ac_vs, sd_vy(inv_xform_i), sd_vec3_muls(ab_vs, sd_vx(inv_xform_i)));
    sd_vec3 vs_xform_j = sd_vec3_fsmadd(ac_vs, sd_vy(inv_xform_j), sd_vec3_muls(ab_vs, sd_vx(inv_xform_j)));

    sd_vec3 origin_nrml = sd_vec3_set(triangle.vs_nrmls[0].x, triangle.vs_nrmls[0].y, triangle.vs_nrmls[0].z);
    sd_vec3 ab_nrml = sd_vec3_sub(sd_vec3_set(triangle.vs_nrmls[1].x, triangle.vs_nrmls[1].y, triangle.vs_nrmls[1].z), origin_nrml);
    sd_vec3 ac_nrml = sd_vec3_sub(sd_vec3_set(triangle.vs_nrmls[2].x, triangle.vs_nrmls[2].y, triangle.vs_nrmls[2].z), origin_nrml);

    sd_vec3 nrml_xform_i = sd_vec3_fsmadd(ac_nrml, sd_vy(inv_xform_i), sd_vec3_muls(ab_nrml, sd_vx(inv_xform_i)));
    sd_vec3 nrml_xform_j = sd_vec3_fsmadd(ac_nrml, sd_vy(inv_xform_j), sd_vec3_muls(ab_nrml, sd_vx(inv_xform_j)));

    sd_vec2 origin_ts = sd_vec2_set(triangle.ts_verts[0].x, triangle.ts_verts[0].y);
    sd_vec2 ab_ts = sd_vec2_sub(sd_vec2_set(triangle.ts_verts[1].x, triangle.ts_verts[1].y), origin_ts);
    sd_vec2 ac_ts = sd_vec2_sub(sd_vec2_set(triangle.ts_verts[2].x, triangle.ts_verts[2].y), origin_ts);
    
    sd_vec2 ts_xform_i = sd_vec2_fsmadd(ac_ts, sd_vy(inv_xform_i), sd_vec2_muls(ab_ts, sd_vx(inv_xform_i)));
    sd_vec2 ts_xform_j = sd_vec2_fsmadd(ac_ts, sd_vy(inv_xform_j), sd_vec2_muls(ab_ts, sd_vx(inv_xform_j)));

    vec3 scalar_nrml = vec3_cross(vec3_sub(triangle.vs_verts[1], triangle.vs_verts[0]), vec3_sub(triangle.vs_verts[2], triangle.vs_verts[0]));
    sd_vec3 nrml = sd_vec3_set(scalar_nrml.x, scalar_nrml.y, scalar_nrml.z);

    for (int i = triangle_bounds[0]; i < triangle_bounds[1]; ++i) {
        int base = i * sd_bounding_length(canvas->width);

        int left = sd_int_loads(tile.scanlines[0], i - tile.top);
        int right = sd_int_loads(tile.scanlines[1], i - tile.top);
        int sd_left = sd_qot(left);
        int sd_right = sd_bounding_length(right);

        for (int j = sd_left; j < sd_right; ++j) {
            sd_vec2 ss = sd_vec2_create(
                sd_float_add(sd_float_range(), sd_float_set(j * sd_length() + 0.5f)),
                sd_float_add(sd_float_set(i), sd_float_set(0.5f))
            );

            sd_vec2 relative = sd_vec2_sub(ss, origin);

            sd_vec3 fragment_vs = sd_vec3_fsmadd(vs_xform_i, sd_vx(relative), origin_vs);
                    fragment_vs = sd_vec3_fsmadd(vs_xform_j, sd_vy(relative), fragment_vs);

            sd_float inv_z = sd_float_rcp(sd_vz(fragment_vs));
            sd_vec3 fragment_nrml;

            if (flags & TX_RASTERIZER_INTERPOLATE_NORMALS) {
                fragment_nrml = sd_vec3_fsmadd(nrml_xform_i, sd_vx(relative), origin_nrml);
                fragment_nrml = sd_vec3_fsmadd(nrml_xform_j, sd_vy(relative), fragment_nrml);
            } else fragment_nrml = nrml;

            fragment_nrml = sd_vec3_normalize(fragment_nrml);

            sd_vec2 fragment_ts = sd_vec2_fsmadd(ts_xform_i, sd_vx(relative), origin_ts);
                    fragment_ts = sd_vec2_fsmadd(ts_xform_j, sd_vy(relative), fragment_ts);

            sd_vec4 col;
            sd_vec3 bg = sd_vec3_load(canvas->color, base + j);
            sd_float bg_z = sd_float_load(canvas->depth, base + j);
            sd_mask mask = sd_float_between(sd_vx(ss), left, right);

            if (flags & TX_RASTERIZER_TEST_DEPTH)
                mask = sd_mask_and(mask, sd_float_gt(inv_z, bg_z));

            if (flags & TX_RASTERIZER_WRITE_DEPTH)
                sd_float_store(canvas->depth, base + j, sd_float_mask_blend(bg_z, inv_z, mask));

            TX_ShaderParams fragment = {
                .bg = &bg,
                .vs = &fragment_vs,
                .nrml = &fragment_nrml,
                .ts = &fragment_ts,
                .vs2ws_xform = { &vs2ws_xform_i, &vs2ws_xform_j, &vs2ws_xform_k }
            };

            for (size_t i = 0; i < triangle.nshaders; ++i)
                col = triangle.shader_pipeline[i](triangle.shader_states[i], col, fragment);

            sd_vec3_store(canvas->color, base + j, sd_vec3_mask_blend(bg, sd_vxyz(col), mask));
        }
    }
}

void SD_VARIANT(TX_ScanPerspective)(ECS_Handle *self, TX_CanvasTile tile, TX_RasterizerFlags flags, TX_TriangleDraw triangle, int triangle_bounds[2]) {
    TX_Rasterizer *rasterizer = ECS_GetComponent(self, TX_Components.Rasterizer);
    TX_Canvas *canvas = ECS_GetComponent(rasterizer->target, TX_Components.Canvas);
    TX_PerspectiveFOV *perspective_fov = ECS_GetComponent(self, TX_Components.PerspectiveFOV);
    xform3 scalar_vs2ws_xform = TX_GetEntityTransform(self);

    sd_vec3 vs2ws_xform_i = sd_vec3_set(scalar_vs2ws_xform.basis.x.x, scalar_vs2ws_xform.basis.x.y, scalar_vs2ws_xform.basis.x.z);
    sd_vec3 vs2ws_xform_j = sd_vec3_set(scalar_vs2ws_xform.basis.y.x, scalar_vs2ws_xform.basis.y.y, scalar_vs2ws_xform.basis.y.z);
    sd_vec3 vs2ws_xform_k = sd_vec3_set(scalar_vs2ws_xform.basis.z.x, scalar_vs2ws_xform.basis.z.y, scalar_vs2ws_xform.basis.z.z);

    sd_vec3 origin = sd_vec3_set(triangle.vs_verts[0].x, triangle.vs_verts[0].y, triangle.vs_verts[0].z);
    sd_vec3 ab = sd_vec3_sub(sd_vec3_set(triangle.vs_verts[1].x, triangle.vs_verts[1].y, triangle.vs_verts[1].z), origin);
    sd_vec3 ac = sd_vec3_sub(sd_vec3_set(triangle.vs_verts[2].x, triangle.vs_verts[2].y, triangle.vs_verts[2].z), origin);

    sd_vec3 nrml = sd_vec3_cross(ab, ac);
    sd_float inv_nrml_disp = sd_float_rcp(sd_vec3_dot(origin, nrml));

    sd_vec3 perp_ab = sd_vec3_cross(nrml, ab);
    sd_vec3 perp_ac = sd_vec3_cross(ac, nrml);

    sd_float inv_pgram_area = sd_float_rcp(sd_vec3_dot(ab, perp_ac));

    sd_vec2 inv_xform_i = sd_vec2_muls(sd_vec2_create(sd_vx(perp_ac), sd_vx(perp_ab)), inv_pgram_area);
    sd_vec2 inv_xform_j = sd_vec2_muls(sd_vec2_create(sd_vy(perp_ac), sd_vy(perp_ab)), inv_pgram_area);
    sd_vec2 inv_xform_k = sd_vec2_muls(sd_vec2_create(sd_vz(perp_ac), sd_vz(perp_ab)), inv_pgram_area);

    sd_vec3 origin_nrml = sd_vec3_set(triangle.vs_nrmls[0].x, triangle.vs_nrmls[0].y, triangle.vs_nrmls[0].z);
    sd_vec3 ab_nrml = sd_vec3_sub(sd_vec3_set(triangle.vs_nrmls[1].x, triangle.vs_nrmls[1].y, triangle.vs_nrmls[1].z), origin_nrml);
    sd_vec3 ac_nrml = sd_vec3_sub(sd_vec3_set(triangle.vs_nrmls[2].x, triangle.vs_nrmls[2].y, triangle.vs_nrmls[2].z), origin_nrml);

    sd_vec3 nrml_xform_i = sd_vec3_fsmadd(ac_nrml, sd_vy(inv_xform_i), sd_vec3_muls(ab_nrml, sd_vx(inv_xform_i)));
    sd_vec3 nrml_xform_j = sd_vec3_fsmadd(ac_nrml, sd_vy(inv_xform_j), sd_vec3_muls(ab_nrml, sd_vx(inv_xform_j)));
    sd_vec3 nrml_xform_k = sd_vec3_fsmadd(ac_nrml, sd_vy(inv_xform_k), sd_vec3_muls(ab_nrml, sd_vx(inv_xform_k)));

    sd_vec2 origin_ts = sd_vec2_set(triangle.ts_verts[0].x, triangle.ts_verts[0].y);
    sd_vec2 ab_ts = sd_vec2_sub(sd_vec2_set(triangle.ts_verts[1].x, triangle.ts_verts[1].y), origin_ts);
    sd_vec2 ac_ts = sd_vec2_sub(sd_vec2_set(triangle.ts_verts[2].x, triangle.ts_verts[2].y), origin_ts);

    sd_vec2 ts_xform_i = sd_vec2_fsmadd(ac_ts, sd_vy(inv_xform_i), sd_vec2_muls(ab_ts, sd_vx(inv_xform_i)));
    sd_vec2 ts_xform_j = sd_vec2_fsmadd(ac_ts, sd_vy(inv_xform_j), sd_vec2_muls(ab_ts, sd_vx(inv_xform_j)));
    sd_vec2 ts_xform_k = sd_vec2_fsmadd(ac_ts, sd_vy(inv_xform_k), sd_vec2_muls(ab_ts, sd_vx(inv_xform_k)));

    sd_vec2 midpoint = sd_vec2_set(canvas->width * 0.5f, canvas->height * 0.5f);
    sd_float normalize_ss = sd_float_mul(sd_float_set(perspective_fov->tan_half_fov), sd_float_rcp(sd_vx(midpoint)));

    for (int i = triangle_bounds[0]; i < triangle_bounds[1]; ++i) {
        int base = i * sd_bounding_length(canvas->width);
        
        int left = sd_int_loads(tile.scanlines[0], i - tile.top);
        int right = sd_int_loads(tile.scanlines[1], i - tile.top);
        int sd_left = sd_qot(left);
        int sd_right = sd_bounding_length(right);

        for (int j = sd_left; j < sd_right; ++j) {
            sd_float fragment_x = sd_float_add(sd_float_range(), sd_float_set(j * sd_length() + 0.5));
            sd_float fragment_y = sd_float_add(sd_float_set(i), sd_float_set(0.5));

            sd_vec2 proj_plane = sd_vec2_muls(sd_vec2_sub(
                sd_vec2_create(fragment_x, sd_vy(midpoint)),
                sd_vec2_create(sd_vx(midpoint), fragment_y)
            ), normalize_ss);

            sd_float inv_z = sd_float_mul(sd_vec3_dot(sd_vec3_create(
                sd_vx(proj_plane),
                sd_vy(proj_plane),
                sd_float_one()
            ), nrml), inv_nrml_disp);

            sd_float fragment_z = sd_float_rcp(inv_z);

            sd_vec3 fragment_vs = sd_vec3_create(
                sd_float_mul(sd_vx(proj_plane), fragment_z),
                sd_float_mul(sd_vy(proj_plane), fragment_z),
                fragment_z
            );

            sd_vec3 relative = sd_vec3_sub(fragment_vs, origin);
            sd_vec3 fragment_nrml;

            if (flags & TX_RASTERIZER_INTERPOLATE_NORMALS) {
                fragment_nrml = sd_vec3_fsmadd(nrml_xform_i, sd_vx(relative), origin_nrml);
                fragment_nrml = sd_vec3_fsmadd(nrml_xform_j, sd_vy(relative), fragment_nrml);
                fragment_nrml = sd_vec3_fsmadd(nrml_xform_k, sd_vz(relative), fragment_nrml);
            } else fragment_nrml = nrml;

            fragment_nrml = sd_vec3_normalize(fragment_nrml);

            sd_vec2 fragment_ts = sd_vec2_fsmadd(ts_xform_i, sd_vx(relative), origin_ts);
                    fragment_ts = sd_vec2_fsmadd(ts_xform_j, sd_vy(relative), fragment_ts);
                    fragment_ts = sd_vec2_fsmadd(ts_xform_k, sd_vz(relative), fragment_ts);

            sd_vec4 col;
            sd_vec3 bg = sd_vec3_load(canvas->color, base + j);
            sd_float bg_z = sd_float_load(canvas->depth, base + j);
            sd_mask mask = sd_float_between(fragment_x, left, right);

            TX_ShaderParams fragment = {
                .bg = &bg,
                .vs = &fragment_vs,
                .nrml = &fragment_nrml,
                .ts = &fragment_ts,
                .vs2ws_xform = { &vs2ws_xform_i, &vs2ws_xform_j, &vs2ws_xform_k }
            };

            if (flags & TX_RASTERIZER_TEST_DEPTH)
                mask = sd_mask_and(mask, sd_float_gt(inv_z, bg_z));

            if (flags & TX_RASTERIZER_WRITE_DEPTH)
                sd_float_store(canvas->depth, base + j, sd_float_mask_blend(bg_z, inv_z, mask));

            for (size_t i = 0; i < triangle.nshaders; ++i)
                col = triangle.shader_pipeline[i](triangle.shader_states[i], col, fragment);

            sd_vec3_store(canvas->color, base + j, sd_vec3_mask_blend(bg, sd_vxyz(col), mask));
        }
    }
}

static void Span(TX_CanvasTile tile, vec2 verts[3], int triangle_bounds[2]) {
    int idxs[] = { 0, 1, 2 };

    /* Sort by ascending y */
    if (verts[idxs[0]].y > verts[idxs[1]].y) swap(&idxs[0], &idxs[1]);
    if (verts[idxs[1]].y > verts[idxs[2]].y) swap(&idxs[1], &idxs[2]);
    if (verts[idxs[0]].y > verts[idxs[1]].y) swap(&idxs[0], &idxs[1]);

    sd_float sd_grad01 = sd_float_set((verts[idxs[1]].x - verts[idxs[0]].x) / (verts[idxs[1]].y - verts[idxs[0]].y));
    sd_float sd_grad12 = sd_float_set((verts[idxs[2]].x - verts[idxs[1]].x) / (verts[idxs[2]].y - verts[idxs[1]].y));
    sd_float sd_grad02 = sd_float_set((verts[idxs[2]].x - verts[idxs[0]].x) / (verts[idxs[2]].y - verts[idxs[0]].y));

    /* True if Long edge is left edge */
    bool le_left = ((idxs[0] + 2) % 3 == idxs[2]);
    size_t sd_top = sd_qot(triangle_bounds[0]);
    size_t sd_bottom = sd_bounding_length(triangle_bounds[1]);

    for (size_t i = sd_top; i < sd_bottom; ++i) {
        sd_float y = sd_float_add(sd_float_range(), sd_float_set(i * sd_length() + 0.5f));
        sd_float offset0 = sd_float_sub(y, sd_float_set(verts[idxs[0]].y));
        sd_float offset1 = sd_float_sub(y, sd_float_set(verts[idxs[1]].y));

        sd_float x01 = sd_float_fmadd(sd_grad01, offset0, sd_float_set(verts[idxs[0]].x));
        sd_float x12 = sd_float_fmadd(sd_grad12, offset1, sd_float_set(verts[idxs[1]].x));
        sd_float x02 = sd_float_fmadd(sd_grad02, offset0, sd_float_set(verts[idxs[0]].x));

        sd_int left = sd_float_to_int(sd_float_add(sd_float_clamp(le_left ? x02 : sd_float_max(x01, x12), sd_float_set(tile.left), sd_float_set(tile.right)), sd_float_set(0.5f)));
        sd_int right = sd_float_to_int(sd_float_add(sd_float_clamp(le_left ? sd_float_min(x01, x12) : x02, sd_float_set(tile.left), sd_float_set(tile.right)), sd_float_set(0.5f)));

        sd_int_store(tile.scanlines[0], i - sd_qot(tile.top), left);
        sd_int_store(tile.scanlines[1], i - sd_qot(tile.top), right);
    }
}

static void DrawTriangle(TX_RasterWorkerData *wd, TX_CanvasTile tile, TX_RasterizerFlags flags, TX_TriangleDraw triangle) {
    float min_y = SDL_min(SDL_min(triangle.ss_verts[0].y, triangle.ss_verts[1].y), triangle.ss_verts[2].y);
    float max_y = SDL_max(SDL_max(triangle.ss_verts[0].y, triangle.ss_verts[1].y), triangle.ss_verts[2].y);

    int triangle_bounds[2] = {
        SDL_clamp((int)(min_y + 0.5f), tile.top, tile.bottom),
        SDL_clamp((int)(max_y + 0.5f), tile.top, tile.bottom)
    };

    Span(tile, triangle.ss_verts, triangle_bounds);
    wd->rasterizer->scan(wd->entity, tile, flags, triangle, triangle_bounds);
}

static void DrawBatch(TX_RasterWorkerData *wd, TX_CanvasTile tile, List(TX_RenderInstance *) *batch, TX_RasterizerFlags flags) {
    /* Draw triangles */
    List_ForEach(batch, instance, {
        TX_MeshFace *faces = instance->geometry->mesh->faces;
        size_t nfaces = instance->geometry->mesh->nfaces;

        for (size_t i = 0; i < nfaces; ++i) {
            vec3 vs_verts[3];

            SDL_memcpy(vs_verts, &(sd_vec3_scalar [3]) {
                sd_vec3_loads(instance->geometry->vs_verts, faces[i].idx_verts[0]),
                sd_vec3_loads(instance->geometry->vs_verts, faces[i].idx_verts[1]),
                sd_vec3_loads(instance->geometry->vs_verts, faces[i].idx_verts[2])
            }, sizeof(vec3 [3]));

            /* Perform near plane clipping */
            vec2 ss_verts[3];
            vec2 clipped[4];
            int nclipped = 0;

            SDL_memcpy(ss_verts, (sd_vec2_scalar [3]) {
                sd_vec2_loads(instance->geometry->ss_verts, faces[i].idx_verts[0]),
                sd_vec2_loads(instance->geometry->ss_verts, faces[i].idx_verts[1]),
                sd_vec2_loads(instance->geometry->ss_verts, faces[i].idx_verts[2]),
            }, sizeof(vec2 [3]));

            for (int j = 0; j < 3; ++j) {
                vec3 curr = vs_verts[j];
                vec3 next = vs_verts[(j + 1) % 3];

                if (curr.z >= wd->rasterizer->near)
                    clipped[nclipped++] = ss_verts[j];

                if ((curr.z < wd->rasterizer->near) != (next.z < wd->rasterizer->near)) {
                    vec3 intercept = intersect_near(curr, next, wd->rasterizer->near);

                    sd_vec2 projected = wd->rasterizer->project(wd->entity,
                        sd_vec3_set(intercept.x, intercept.y, wd->rasterizer->near),
                        sd_vec2_set(wd->canvas->width * 0.5f, wd->canvas->height * 0.5f)
                    );

                    sd_vec2_scalar projected_scalar = sd_vec2_loads(&projected, 0);
                    SDL_memcpy(clipped + nclipped++, &projected_scalar, sizeof(vec2));
                }
            }

            /* Triangle fan clipped verticies */
            for (int j = 1; j < nclipped - 1; ++j) {
                /* Compute extremes */
                float min_x = SDL_min(clipped[0].x, SDL_min(clipped[j].x, clipped[j + 1].x));
                float max_x = SDL_max(clipped[0].x, SDL_max(clipped[j].x, clipped[j + 1].x));
                float min_y = SDL_min(clipped[0].y, SDL_min(clipped[j].y, clipped[j + 1].y));
                float max_y = SDL_max(clipped[0].y, SDL_max(clipped[j].y, clipped[j + 1].y));

                /* Cull off-screen triangles */
                if (min_x > tile.right || max_x < tile.left || min_y > tile.bottom || max_y < tile.top)
                    continue;

                bool verts_cw = vec2_dot(
                    vec2_orthogonal(vec2_sub(clipped[j], clipped[0])),
                    vec2_sub(clipped[j + 1], clipped[0])
                ) > 0;

                if (flags & TX_RASTERIZER_CULL_BACKFACE && !verts_cw)
                    continue;

                TX_TriangleDraw triangle = {
                    .shader_pipeline = instance->shader_pipeline,
                    .shader_states = instance->shader_states,
                    .nshaders = instance->nshaders
                };

                SDL_memcpy(triangle.vs_verts, (vec3 [3]) { vs_verts[0], vs_verts[1 + !verts_cw], vs_verts[1 + verts_cw] }, sizeof(vec3 [3]));
                SDL_memcpy(triangle.ss_verts, (vec2 [3]) { clipped[0], clipped[j + !verts_cw], clipped[j + verts_cw] }, sizeof(vec2 [3]));

                if (instance->geometry->vs_nrmls)
                    SDL_memcpy(triangle.vs_nrmls, (sd_vec3_scalar [3]) {
                        sd_vec3_loads(instance->geometry->vs_nrmls, faces[i].idx_verts[0]),
                        sd_vec3_loads(instance->geometry->vs_nrmls, faces[i].idx_verts[1 + !verts_cw]),
                        sd_vec3_loads(instance->geometry->vs_nrmls, faces[i].idx_verts[1 + verts_cw])
                    }, sizeof(vec3 [3]));

                if (instance->geometry->mesh->ts_verts)
                    SDL_memcpy(triangle.ts_verts, (vec2 [3]) {
                        instance->geometry->mesh->ts_verts[faces[i].idx_tverts[0]],
                        instance->geometry->mesh->ts_verts[faces[i].idx_tverts[1 + !verts_cw]],
                        instance->geometry->mesh->ts_verts[faces[i].idx_tverts[1 + verts_cw]]
                    }, sizeof(vec2 [3]));

                DrawTriangle(wd, tile, flags, triangle);
            }
        }
    });
}

static void RenderToCanvasTile(TX_RasterWorkerData *wd, TX_CanvasTile tile) {
    /*
     * changes from kdr that make sorting more difficult
     * kdr's geometry buffers stored all triangles from all meshes in a single buffer
     * tx changed this design for easier vectorization and to allow the same mesh to be instanced multiple times with different shaders
     * if a given call to DrawBatch needs to sort triangles, a temp list of all triangles from all instances in the batch will need to be created and sorted, then drawn
     */

    size_t sd_width = sd_bounding_length(wd->canvas->width);

    /* Reset depth */
    for (int i = tile.top; i < tile.bottom; ++i)
        for (size_t j = sd_qot(tile.left); j < sd_bounding_length(tile.right); ++j)
            sd_float_store(wd->canvas->depth, i * sd_width + j, sd_float_zero());

    /* Draw geometry in batches, according to render order and rasterizer flags */
    for (size_t i = 0; i < List_Length(wd->world->render_batches); ++i) {
        for (int flags = 0; flags < TX_RASTERIZER_FLAG_COMBINATIONS; ++flags) {
            List(TX_RenderInstance *) *flag_batch = List_Get(wd->world->render_batches, i)[flags];

            if (flag_batch)
                DrawBatch(wd, tile, flag_batch, flags);
        }
    }
}

int SD_VARIANT(TX_RasterWorker)(void *data) {
    TX_RasterWorkerData *wd = data;
    int tile_index;
loop:
    SDL_WaitSemaphore(wd->worker_wake);

    if (wd->exit)
        goto exit;

    while ((tile_index = SDL_AtomicIncRef(wd->tile_ref)) < wd->canvas->ntiles) {
        TX_CanvasTile tile = wd->canvas->tiles[tile_index];
        RenderToCanvasTile(wd, tile);
    }

    SDL_SignalSemaphore(wd->worker_rest);
    goto loop;
exit:
    return 0;
}

void SD_VARIANT(TX_RenderWorld)(ECS_Handle *self) {
    TX_Rasterizer *rasterizer = ECS_GetComponent(self, TX_Components.Rasterizer);
    TX_World *world = ECS_GetComponent(rasterizer->world, TX_Components.World);
    TX_Canvas *canvas = ECS_GetComponent(rasterizer->target, TX_Components.Canvas);

    /* Transform registered world geometry */
    List(TX_WorldGeometry *) *geometry = world->geometry;
    xform3 cam_xform = TX_GetEntityTransform(self);

    xform3 ws2vs_xform = {
        mat3x3_transpose(cam_xform.basis),
        vec3_mul(mat3x3_mul(mat3x3_transpose(cam_xform.basis), cam_xform.translation), -1)
    };

    TX_TransformEntity(rasterizer->world, ws2vs_xform);

    List_ForEach(geometry, wg, {
        size_t sd_count = sd_bounding_length(wg->mesh->nverts);

        sd_vec3 translation = sd_vec3_set(
            wg->xform.translation.x,
            wg->xform.translation.y,
            wg->xform.translation.z
        );

        sd_vec3 sd_xform_i = sd_vec3_set(wg->xform.basis.x.x, wg->xform.basis.x.y, wg->xform.basis.x.z);
        sd_vec3 sd_xform_j = sd_vec3_set(wg->xform.basis.y.x, wg->xform.basis.y.y, wg->xform.basis.y.z);
        sd_vec3 sd_xform_k = sd_vec3_set(wg->xform.basis.z.x, wg->xform.basis.z.y, wg->xform.basis.z.z);

        for (size_t i = 0; i < sd_count; ++i) {
            sd_vec3 ws_vert = sd_vec3_load(wg->mesh->ws_verts, i);

            sd_vec3 vs_vert = sd_vec3_fsmadd(sd_xform_i, sd_vx(ws_vert), translation);
                    vs_vert = sd_vec3_fsmadd(sd_xform_j, sd_vy(ws_vert), vs_vert);
                    vs_vert = sd_vec3_fsmadd(sd_xform_k, sd_vz(ws_vert), vs_vert);

            sd_vec3_store(wg->vs_verts, i, vs_vert);

            if (wg->vs_nrmls) {
                sd_vec3 ws_nrml = sd_vec3_load(wg->mesh->ws_nrmls, i);

                sd_vec3 vs_nrml = sd_vec3_muls(sd_xform_i, sd_vx(ws_nrml));
                        vs_nrml = sd_vec3_fsmadd(sd_xform_j, sd_vy(ws_nrml), vs_nrml);
                        vs_nrml = sd_vec3_fsmadd(sd_xform_k, sd_vz(ws_nrml), vs_nrml);

                sd_vec3_store(wg->vs_nrmls, i, sd_vec3_normalize(vs_nrml));
            }

            sd_vec2_store(wg->ss_verts, i, rasterizer->project(self, vs_vert, sd_vec2_set(canvas->width * 0.5f, canvas->height * 0.5f)));
        }
    });

    int nproc = SDL_GetNumLogicalCPUCores();
    TX_RasterThreadPool *pool = rasterizer->thread_pool;
    SDL_SetAtomicInt(&pool->tile, 0);

    for (int i = 0; i < nproc; ++i) {
        pool->worker_data[i].entity = self;
        pool->worker_data[i].rasterizer = rasterizer;
        pool->worker_data[i].canvas = canvas;
        pool->worker_data[i].world = world;
    }

    for (int i = 0; i < nproc; ++i)
        SDL_SignalSemaphore(pool->worker_wake);

    for (int i = 0; i < nproc; ++i)
        SDL_WaitSemaphore(pool->worker_rest);
}

#ifndef SD_SRC_VARIANT

void TX_AttachRasterizer(ECS_Handle *self, ECS_Component(void) *component) {
    TX_Rasterizer *rasterizer = ECS_GetComponent(self, component);
    rasterizer->world = ECS_GetAncestorWithComponent(self, TX_Components.World, true);
    rasterizer->target = ECS_GetAncestorWithComponent(self, TX_Components.Canvas, true);

    int nproc = SDL_GetNumLogicalCPUCores();
    TX_RasterThreadPool *pool = SDL_malloc(sizeof(TX_RasterThreadPool));
    rasterizer->thread_pool = pool;
    pool->threads = SDL_malloc(sizeof(SDL_Thread *) * nproc);
    pool->worker_data = SDL_malloc(sizeof(TX_RasterWorkerData) * nproc);
    pool->worker_wake = SDL_CreateSemaphore(0);
    pool->worker_rest = SDL_CreateSemaphore(0);

    for (int i = 0; i < nproc; ++i) {
        pool->worker_data[i] = (TX_RasterWorkerData) {
            .worker_wake = pool->worker_wake,
            .worker_rest = pool->worker_rest,
            .tile_ref = &pool->tile,
            .exit = false
        };

        pool->threads[i] = SDL_CreateThread(SD_SELECT(TX_RasterWorker), "rasterworker", pool->worker_data + i);
    }
}

void TX_DetachRasterizer(ECS_Handle *self, ECS_Component(void) *component) {
    TX_Rasterizer *rasterizer = ECS_GetComponent(self, component);
    TX_RasterThreadPool *pool = rasterizer->thread_pool;
    int nproc = SDL_GetNumLogicalCPUCores();

    for (int i = 0; i < nproc; ++i)
        pool->worker_data[i].exit = true;

    for (int i = 0; i < nproc; ++i)
        SDL_SignalSemaphore(pool->worker_wake);

    for (int i = 0; i < nproc; ++i)
        SDL_WaitThread(pool->threads[i], nullptr);

    SDL_DestroySemaphore(pool->worker_rest);
    SDL_DestroySemaphore(pool->worker_wake);
    SDL_free(pool->worker_data);
    SDL_free(pool->threads);
    SDL_free(pool);
}

void TX_InitRasterizer(void *component, void *args) {
    TX_Rasterizer *rasterizer = component, *rasterizer_args = args;

    *rasterizer = (TX_Rasterizer) {
        .project = rasterizer_args->project,
        .scan = rasterizer_args->scan,
        .near = rasterizer_args->near
    };
}

void TX_SetPerspectiveFOV(ECS_Handle *self, float fov) {
    TX_PerspectiveFOV *perspective_fov = ECS_GetComponent(self, TX_Components.PerspectiveFOV);
    perspective_fov->fov = fov;
    perspective_fov->tan_half_fov = SDL_tanf(fov / 2);
}

void TX_InitPerspectiveFOV(void *component, void *args) {
    TX_PerspectiveFOV *perspective_fov = component;
    float *fov = args;

    perspective_fov->fov = *fov;
    perspective_fov->tan_half_fov = SDL_tanf(*fov / 2);
}

#endif /* SD_SRC_VARIANT */
