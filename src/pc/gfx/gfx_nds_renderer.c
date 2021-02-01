#ifdef TARGET_NDS

#include <nds.h>

#include "gfx_cc.h"
#include "gfx_rendering_api.h"

static void glVertex4v16(v16 x, v16 y, v16 z, v16 w) {
    const m4x4 matrix = {
        x, 0, 0, 0,
        0, y, 0, 0,
        0, 0, z, 0,
        0, 0, 0, w
    };

    glMatrixMode(GL_PROJECTION);
    glLoadMatrix4x4(&matrix);
    glVertex3v16(inttov16(1), inttov16(1), inttov16(1));
}

struct ShaderProgram {
    uint32_t program_id;
    uint32_t shader_id;
    struct CCFeatures cc_features;
};

static struct ShaderProgram shader_programs[32];
static uint8_t shader_program_count = 0;
static uint8_t cur_shader_program = 0;

static bool cur_depth_test = true;

static bool gfx_nds_renderer_z_is_from_0_to_1(void) {
    return false;
}

static void gfx_nds_renderer_unload_shader(struct ShaderProgram *old_prg) {
}

static void gfx_nds_renderer_load_shader(struct ShaderProgram *new_prg) {
    cur_shader_program = new_prg->program_id;
}

static struct ShaderProgram *gfx_nds_renderer_create_and_load_new_shader(uint32_t shader_id) {
    struct ShaderProgram *prg = &shader_programs[shader_program_count];
    prg->program_id = shader_program_count++;
    prg->shader_id = shader_id;
    gfx_cc_get_features(shader_id, &prg->cc_features);
    return prg;
}

static struct ShaderProgram *gfx_nds_renderer_lookup_shader(uint32_t shader_id) {
    for (size_t i = 0; i < shader_program_count; i++) {
        if (shader_programs[i].shader_id == shader_id) {
            return &shader_programs[i];
        }
    }
    return NULL;
}

static void gfx_nds_renderer_shader_get_info(struct ShaderProgram *prg, uint8_t *num_inputs, bool used_textures[2]) {
    *num_inputs = prg->cc_features.num_inputs;
    used_textures[0] = prg->cc_features.used_textures[0];
    used_textures[1] = prg->cc_features.used_textures[0];
}

static uint32_t gfx_nds_renderer_new_texture(void) {
    return 0;
}

static void gfx_nds_renderer_select_texture(int tile, uint32_t texture_id) {
}

static void gfx_nds_renderer_upload_texture(const uint8_t *rgba32_buf, int width, int height) {
}

static void gfx_nds_renderer_set_sampler_parameters(int tile, bool linear_filter, uint32_t cms, uint32_t cmt) {
}

static void gfx_nds_renderer_set_depth_test(bool depth_test) {
    cur_depth_test = depth_test;
}

static void gfx_nds_renderer_set_depth_mask(bool z_upd) {
}

static void gfx_nds_renderer_set_zmode_decal(bool zmode_decal) {
}

static void gfx_nds_renderer_set_viewport(int x, int y, int width, int height) {
    //glViewport(x, y, width, height);
}

static void gfx_nds_renderer_set_scissor(int x, int y, int width, int height) {
}

static void gfx_nds_renderer_set_use_alpha(bool use_alpha) {
    if (use_alpha) {
        glEnable(GL_BLEND);
    } else {
        glDisable(GL_BLEND);
    }
}

static void gfx_nds_renderer_draw_triangles(float buf_vbo[], size_t buf_vbo_len, size_t buf_vbo_num_tris) {
    if (!cur_depth_test) return;

    const struct CCFeatures *cc_features = &shader_programs[cur_shader_program].cc_features;
    const size_t t = buf_vbo_len / buf_vbo_num_tris;
    const size_t v = t / 3;

    if (cc_features->num_inputs == 0)
    {
        glBegin(GL_TRIANGLE);
        for (size_t i = 0; i < buf_vbo_num_tris; i++) {
            glColor3b(0xFF, 0xFF, 0xFF);
            glVertex4v16(buf_vbo[i * t + v * 0 + 0], buf_vbo[i * t + v * 0 + 1], buf_vbo[i * t + v * 0 + 2], buf_vbo[i * t + v * 0 + 3]);
            glVertex4v16(buf_vbo[i * t + v * 1 + 0], buf_vbo[i * t + v * 1 + 1], buf_vbo[i * t + v * 1 + 2], buf_vbo[i * t + v * 1 + 3]);
            glVertex4v16(buf_vbo[i * t + v * 2 + 0], buf_vbo[i * t + v * 2 + 1], buf_vbo[i * t + v * 2 + 2], buf_vbo[i * t + v * 2 + 3]);
        }
        glEnd();
    }
    else
    {
        const size_t c = cc_features->opt_alpha ? 4 : 3;
        size_t o = (cc_features->used_textures[0] || cc_features->used_textures[1]) ? 2 : 0;

        if (cc_features->num_inputs > 1)
        {
            const float r1 = buf_vbo[4 + o];
            const float g1 = buf_vbo[5 + o];
            const float b1 = buf_vbo[6 + o];
            const float r2 = buf_vbo[4 + o + c];
            const float g2 = buf_vbo[5 + o + c];
            const float b2 = buf_vbo[6 + o + c];

            bool first_constant = false;

            for (size_t i = 1; i < buf_vbo_num_tris * 3; i++) {
                if (r1 != buf_vbo[i * v + 4 + o] || g1 != buf_vbo[i * v + 5 + o] || b1 != buf_vbo[i * v + 6 + o]) {
                    break;
                }

                if (r2 != buf_vbo[i * v + 4 + o + c] || g2 != buf_vbo[i * v + 5 + o + c] || b2 != buf_vbo[i * v + 6 + o + c]) {
                    first_constant = true;
                    break;
                }
            }

            if (first_constant) {
                o += c;
            }
        }

        glBegin(GL_TRIANGLE);
        for (size_t i = 0; i < buf_vbo_num_tris; i++) {
            glColor3f(buf_vbo[i * t + v * 0 + 4 + o], buf_vbo[i * t + v * 0 + 5 + o], buf_vbo[i * t + v * 0 + 6 + o]);
            glVertex4v16(buf_vbo[i * t + v * 0 + 0], buf_vbo[i * t + v * 0 + 1], buf_vbo[i * t + v * 0 + 2], buf_vbo[i * t + v * 0 + 3]);
            glColor3f(buf_vbo[i * t + v * 1 + 4 + o], buf_vbo[i * t + v * 1 + 5 + o], buf_vbo[i * t + v * 1 + 6 + o]);
            glVertex4v16(buf_vbo[i * t + v * 1 + 0], buf_vbo[i * t + v * 1 + 1], buf_vbo[i * t + v * 1 + 2], buf_vbo[i * t + v * 1 + 3]);
            glColor3f(buf_vbo[i * t + v * 2 + 4 + o], buf_vbo[i * t + v * 2 + 5 + o], buf_vbo[i * t + v * 2 + 6 + o]);
            glVertex4v16(buf_vbo[i * t + v * 2 + 0], buf_vbo[i * t + v * 2 + 1], buf_vbo[i * t + v * 2 + 2], buf_vbo[i * t + v * 2 + 3]);
        }
        glEnd();
    }
}

static void gfx_nds_renderer_init(void) {
    glInit();
 
    glViewport(0, 0, 255, 191);
    glClearColor(0, 0, 0, 31);
    glClearDepth(0x7FFF);
    glPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE);
    glEnable(GL_ANTIALIAS);
}

static void gfx_nds_renderer_on_resize(void) {
}

static void gfx_nds_renderer_start_frame(void) {
}

static void gfx_nds_renderer_end_frame(void) {
    glFlush(0);
}

static void gfx_nds_renderer_finish_render(void) {
}

struct GfxRenderingAPI gfx_nds_renderer = {
    gfx_nds_renderer_z_is_from_0_to_1,
    gfx_nds_renderer_unload_shader,
    gfx_nds_renderer_load_shader,
    gfx_nds_renderer_create_and_load_new_shader,
    gfx_nds_renderer_lookup_shader,
    gfx_nds_renderer_shader_get_info,
    gfx_nds_renderer_new_texture,
    gfx_nds_renderer_select_texture,
    gfx_nds_renderer_upload_texture,
    gfx_nds_renderer_set_sampler_parameters,
    gfx_nds_renderer_set_depth_test,
    gfx_nds_renderer_set_depth_mask,
    gfx_nds_renderer_set_zmode_decal,
    gfx_nds_renderer_set_viewport,
    gfx_nds_renderer_set_scissor,
    gfx_nds_renderer_set_use_alpha,
    gfx_nds_renderer_draw_triangles,
    gfx_nds_renderer_init,
    gfx_nds_renderer_on_resize,
    gfx_nds_renderer_start_frame,
    gfx_nds_renderer_end_frame,
    gfx_nds_renderer_finish_render
};

#endif
