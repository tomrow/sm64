#ifdef TARGET_NDS

#include <nds.h>

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

static bool cur_depth_test = true;

static bool gfx_nds_renderer_z_is_from_0_to_1(void) {
    return false;
}

static void gfx_nds_renderer_unload_shader(struct ShaderProgram *old_prg) {
}

static void gfx_nds_renderer_load_shader(struct ShaderProgram *new_prg) {
}

static struct ShaderProgram *gfx_nds_renderer_create_and_load_new_shader(uint32_t shader_id) {
    return NULL;
}

static struct ShaderProgram *gfx_nds_renderer_lookup_shader(uint32_t shader_id) {
    return NULL;
}

static void gfx_nds_renderer_shader_get_info(struct ShaderProgram *prg, uint8_t *num_inputs, bool used_textures[2]) {
    *num_inputs = 0;
    used_textures[0] = false;
    used_textures[1] = false;
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

    const size_t s = buf_vbo_len / buf_vbo_num_tris;
    if (s % 12 != 0) return;

    glMatrixMode(GL_PROJECTION);

    glBegin(GL_TRIANGLE);
    for (size_t i = 0; i < buf_vbo_num_tris; i++) {
        glColor3b((i & 0x1F) << 3, (i & 0x1F) << 3, (i & 0x1F) << 3);
        glVertex4v16(buf_vbo[i * s + (s / 3) * 0 + 0], buf_vbo[i * s + (s / 3) * 0 + 1], buf_vbo[i * s + (s / 3) * 0 + 2], buf_vbo[i * s + (s / 3) * 0 + 3]);
        glVertex4v16(buf_vbo[i * s + (s / 3) * 1 + 0], buf_vbo[i * s + (s / 3) * 1 + 1], buf_vbo[i * s + (s / 3) * 1 + 2], buf_vbo[i * s + (s / 3) * 1 + 3]);
        glVertex4v16(buf_vbo[i * s + (s / 3) * 2 + 0], buf_vbo[i * s + (s / 3) * 2 + 1], buf_vbo[i * s + (s / 3) * 2 + 2], buf_vbo[i * s + (s / 3) * 2 + 3]);
    }
    glEnd();
}

static void gfx_nds_renderer_init(void) {
    glInit();
 
    glViewport(0, 0, 255, 191);
    glClearColor(0, 0, 0, 31);
    glClearDepth(0x7FFF);
    glPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE);
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
