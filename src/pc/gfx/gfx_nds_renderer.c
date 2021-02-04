#ifdef TARGET_NDS

// Hack for type redefinitions in libnds
#define u64 _u64
#define s64 _s64
#define u32 _u32
#define vu32 _vu32
#define vs32 _vs32
#define s32 _s32
#define u16 _u16
#define s16 _s16
#define u8 _u8
#define s8 _s8
#include <nds.h>
#undef u64
#undef s64
#undef u32
#undef vu32
#undef vs32
#undef s32
#undef u16
#undef s16
#undef u8
#undef s8

#include <PR/gbi.h>
#include <stdio.h>

#include "gfx_cc.h"
#include "gfx_rendering_api.h"

#define TEXTURE_POOL_SIZE 1024
#define MAX_UI_ELEMENTS 128

struct ShaderProgram {
    uint8_t program_id;
    uint32_t shader_id;
    struct CCFeatures cc_features;
};

struct TextureInfo {
    int texture;
    int size_x;
    int size_y;
    int param;
    int format;
    uint8_t *data;
};

static struct ShaderProgram shader_programs[32];
static uint8_t shader_program_count;
static uint8_t cur_shader_program;

static struct TextureInfo texture_infos[TEXTURE_POOL_SIZE];
static uint16_t texture_info_count;
static uint16_t cur_texture_info;
static int no_texture;

static uint16_t texture_fifo[TEXTURE_POOL_SIZE];
static uint16_t texture_fifo_start;
static uint16_t texture_fifo_end;

static bool cur_depth_test;
static bool background;
static v16 z_depth;

static bool cur_zmode_decal;
static int polygon_id;

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
    used_textures[1] = prg->cc_features.used_textures[1];
}

static uint32_t gfx_nds_renderer_new_texture(void) {
    if (texture_info_count >= TEXTURE_POOL_SIZE) {
        printf("Texture pool full; old textures will be overwritten!\n");
        texture_info_count = 0;
    }

    struct TextureInfo *info = &texture_infos[texture_info_count];
    if (!info->texture) {
        glGenTextures(1, &info->texture);
    }

    return texture_info_count++;
}

static void gfx_nds_renderer_select_texture(int tile, uint32_t texture_id) {
    struct TextureInfo *info = &texture_infos[texture_id];
    cur_texture_info = texture_id;

    if (!info->texture) {
        glGenTextures(1, &info->texture);
        glBindTexture(GL_TEXTURE_2D, info->texture);

        if (info->data) {
            uint16_t i = texture_fifo_end;
            for (; !glTexImage2D(GL_TEXTURE_2D, 0, info->format, info->size_x, info->size_y, 0, TEXGEN_TEXCOORD, info->data); i = (i + 1) % TEXTURE_POOL_SIZE) {
                if (i != texture_id && texture_infos[i].texture) {
                    glDeleteTextures(1, &texture_infos[i].texture);
                    texture_infos[i].texture = 0;
                }
            }
            texture_fifo_end = i;

            texture_fifo[texture_fifo_start] = texture_id;
            texture_fifo_start = (texture_fifo_start + 1) % TEXTURE_POOL_SIZE;
        }
    }

    glBindTexture(GL_TEXTURE_2D, info->texture);
    glTexParameter(GL_TEXTURE_2D, info->param);
}

static void gfx_nds_renderer_upload_texture(const uint8_t *rgba32_buf, int width, int height) {
    struct TextureInfo *info = &texture_infos[cur_texture_info];

    if (info->data) {
        free(info->data);
        info->data = NULL;
    }

    switch (width) {
        case   8: info->size_x = TEXTURE_SIZE_8;   break;
        case  16: info->size_x = TEXTURE_SIZE_16;  break;
        case  32: info->size_x = TEXTURE_SIZE_32;  break;
        case  64: info->size_x = TEXTURE_SIZE_64;  break;
        case 128: info->size_x = TEXTURE_SIZE_128; break;
        default: return;
    };

    switch (height) {
        case   8: info->size_y = TEXTURE_SIZE_8;   break;
        case  16: info->size_y = TEXTURE_SIZE_16;  break;
        case  32: info->size_y = TEXTURE_SIZE_32;  break;
        case  64: info->size_y = TEXTURE_SIZE_64;  break;
        case 128: info->size_y = TEXTURE_SIZE_128; break;
        default: return;
    };

    info->param = glGetTexParameter();

    info->format = GL_RGBA;
    for (int i = 0; i < width * height; i++) {
        const uint8_t a = ((uint32_t*)rgba32_buf)[i] >> 24;
        if (a > 0 && a < 0xFF) {
            info->format = GL_RGB8_A5;
            break;
        }
    }

    const int size = width * height * ((info->format == GL_RGBA) ? sizeof(uint16_t) : sizeof(uint8_t));
    info->data = (uint8_t*)malloc(size);

    if (info->format == GL_RGBA) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                const uint32_t color = ((uint32_t*)rgba32_buf)[y * width + x];
                const uint8_t r = ((color >>  0) & 0xFF) >> 3;
                const uint8_t g = ((color >>  8) & 0xFF) >> 3;
                const uint8_t b = ((color >> 16) & 0xFF) >> 3;
                const uint8_t a = ((color >> 24) & 0xFF) ? 1 : 0;
                ((uint16_t*)info->data)[y * width + x] = (a << 15) | (b << 10) | (g << 5) | r;
            }
        }
    }
    else {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                const uint32_t color = ((uint32_t*)rgba32_buf)[y * width + x];
                const uint8_t i = ((color >>  0) & 0xFF) >> 5;
                const uint8_t a = ((color >> 24) & 0xFF) >> 3;
                info->data[y * width + x] = (a << 3) | i;
            }
        }
    }

    DC_FlushRange(info->data, size);

    uint16_t i = texture_fifo_end;
    for (; !glTexImage2D(GL_TEXTURE_2D, 0, info->format, info->size_x, info->size_y, 0, TEXGEN_TEXCOORD, info->data); i = (i + 1) % TEXTURE_POOL_SIZE) {
        if (i != cur_texture_info && texture_infos[i].texture) {
            glDeleteTextures(1, &texture_infos[i].texture);
            texture_infos[i].texture = 0;
        }
    }
    texture_fifo_end = i;

    texture_fifo[texture_fifo_start] = cur_texture_info;
    texture_fifo_start = (texture_fifo_start + 1) % TEXTURE_POOL_SIZE;
}

static void gfx_nds_renderer_set_sampler_parameters(int tile, bool linear_filter, uint32_t cms, uint32_t cmt) {
    int param = 0;

    if (!(cms & G_TX_CLAMP)) {
        param |= GL_TEXTURE_WRAP_S;
        if (cms & G_TX_MIRROR) {
            param |= GL_TEXTURE_FLIP_S;
        }
    }

    if (!(cmt & G_TX_CLAMP)) {
        param |= GL_TEXTURE_WRAP_T;
        if (cmt & G_TX_MIRROR) {
            param |= GL_TEXTURE_FLIP_T;
        }
    }

    texture_infos[cur_texture_info].param = param;
    glTexParameter(GL_TEXTURE_2D, param);
}

static void gfx_nds_renderer_set_depth_test(bool depth_test) {
    cur_depth_test = depth_test;
}

static void gfx_nds_renderer_set_depth_mask(bool z_upd) {
}

static void gfx_nds_renderer_set_zmode_decal(bool zmode_decal) {
    cur_zmode_decal = zmode_decal;
}

static void gfx_nds_renderer_set_viewport(int x, int y, int width, int height) {
    glViewport(x, y, width - 1, height - 1);
}

static void gfx_nds_renderer_set_scissor(int x, int y, int width, int height) {
}

static void gfx_nds_renderer_set_use_alpha(bool use_alpha) {
}

static void gfx_nds_renderer_push_vtx(float x, float y, float z, float w) {
    // 2D elements are drawn with depth test disabled, which the DS doesn't support
    // This is a hack that relies on the assumption that background elements are pushed first, and foreground last
    if (!cur_depth_test) {
        x *= 0x1000;
        y *= 0x1000;
        z = (--z_depth) / 6;
        w *= 0x1000;
    }
    else if (background) {
        z_depth = (MAX_UI_ELEMENTS - 0x1000) * 6;
        background = false;
    }

    // Adjust the Z value to reduce Z-fighting
    if (cur_zmode_decal) {
        z -= 5;
    }

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

static void gfx_nds_renderer_draw_triangles(float buf_vbo[], size_t buf_vbo_len, size_t buf_vbo_num_tris) {
    const struct CCFeatures *cc_features = &shader_programs[cur_shader_program].cc_features;
    const size_t t = buf_vbo_len / buf_vbo_num_tris;
    const size_t v = t / 3;
    const size_t c = cc_features->opt_alpha ? 4 : 3;
    
    bool has_color = (cc_features->num_inputs > 0);
    bool has_texture = (cc_features->used_textures[0] || cc_features->used_textures[1]);
    size_t o = has_texture ? 2 : 0;

    int poly_fmt = POLY_CULL_NONE;
    bool alpha_max = false;

    if (has_texture) {
        if (cc_features->do_single[0]) {
            poly_fmt |= POLY_MODULATION;
            has_color = false;

            if ((cc_features->do_single[1] || cc_features->color_alpha_same) || !cc_features->do_multiply[1]) {
                alpha_max = true;
            }
        }
        else if (cc_features->do_multiply[0]) {
            poly_fmt |= POLY_MODULATION;

            if ((cc_features->do_single[1] && !cc_features->color_alpha_same) || !(cc_features->do_multiply[1] || cc_features->color_alpha_same)) {
                alpha_max = true;

                // Hack to prevent some levels from being pitch-black
                if (!cc_features->do_mix[1]) {
                    poly_fmt |= POLY_DECAL;
                }
            }
        }
        else if (cc_features->do_mix[0]) {
            poly_fmt |= POLY_DECAL;
        }
        else {
            has_texture = false;
        }
    }

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

        // Hack to hide goddard's texture since it can't be properly blended
        has_texture = false;
    }

    if (!has_color) {
        glColor3b(0xFF, 0xFF, 0xFF);
    }

    if (!has_texture) {
        glBindTexture(GL_TEXTURE_2D, no_texture);
    }

    if (has_color && cc_features->opt_alpha) {
        polygon_id = (polygon_id + 1) % 64;
    }

    glPolyFmt(poly_fmt | POLY_ALPHA(31));
    glBegin(GL_TRIANGLE);

    for (size_t i = 0; i < buf_vbo_num_tris; i++) {
        if (has_color && cc_features->opt_alpha) {
            const int alpha = alpha_max ? 31 : (buf_vbo[i * t + v * 0 + 7 + o] * 31);
            if (alpha == 0) continue;

            glPolyFmt(poly_fmt | POLY_ALPHA(alpha) | POLY_ID(polygon_id));
            glBegin(GL_TRIANGLE);
        }

        if (has_texture) glTexCoord2f(buf_vbo[i * t + v * 0 + 4], buf_vbo[i * t + v * 0 + 5]);
        if (has_color) glColor3f(buf_vbo[i * t + v * 0 + 4 + o], buf_vbo[i * t + v * 0 + 5 + o], buf_vbo[i * t + v * 0 + 6 + o]);
        gfx_nds_renderer_push_vtx(buf_vbo[i * t + v * 0 + 0], buf_vbo[i * t + v * 0 + 1], buf_vbo[i * t + v * 0 + 2], buf_vbo[i * t + v * 0 + 3]);

        if (has_texture) glTexCoord2f(buf_vbo[i * t + v * 1 + 4], buf_vbo[i * t + v * 1 + 5]);
        if (has_color) glColor3f(buf_vbo[i * t + v * 1 + 4 + o], buf_vbo[i * t + v * 1 + 5 + o], buf_vbo[i * t + v * 1 + 6 + o]);
        gfx_nds_renderer_push_vtx(buf_vbo[i * t + v * 1 + 0], buf_vbo[i * t + v * 1 + 1], buf_vbo[i * t + v * 1 + 2], buf_vbo[i * t + v * 1 + 3]);

        if (has_texture) glTexCoord2f(buf_vbo[i * t + v * 2 + 4], buf_vbo[i * t + v * 2 + 5]);
        if (has_color) glColor3f(buf_vbo[i * t + v * 2 + 4 + o], buf_vbo[i * t + v * 2 + 5 + o], buf_vbo[i * t + v * 2 + 6 + o]);
        gfx_nds_renderer_push_vtx(buf_vbo[i * t + v * 2 + 0], buf_vbo[i * t + v * 2 + 1], buf_vbo[i * t + v * 2 + 2], buf_vbo[i * t + v * 2 + 3]);
    }

    glBindTexture(GL_TEXTURE_2D, texture_infos[cur_texture_info].texture);
}

static void gfx_nds_renderer_init(void) {
    vramSetBankA(VRAM_A_TEXTURE);
    vramSetBankB(VRAM_B_TEXTURE);
    vramSetBankD(VRAM_D_TEXTURE);
    vramSetBankE(VRAM_E_TEX_PALETTE);

    glInit();
    glClearColor(0, 0, 0, 31);
    glClearDepth(GL_MAX_DEPTH);
    glEnable(GL_ANTIALIAS);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

    glGenTextures(1, &no_texture);
    glBindTexture(GL_TEXTURE_2D, no_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_NOTEXTURE, 0, 0, 0, TEXGEN_TEXCOORD, NULL);

    uint16_t intensity[8];
    for (int i = 0; i < 8; i++) {
        intensity[i] = ((i << 2) << 10) | ((i << 2) << 5) | (i << 2);
    }
    glColorTableEXT(GL_TEXTURE_2D, 0, 8, 0, 0, intensity);
}

static void gfx_nds_renderer_on_resize(void) {
}

static void gfx_nds_renderer_start_frame(void) {
    background = true;
    z_depth = 0x1000 * 6;
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
