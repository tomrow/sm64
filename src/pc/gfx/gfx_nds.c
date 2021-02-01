#ifdef TARGET_NDS

#include <nds.h>

#include "gfx_window_manager_api.h"

static void gfx_nds_init(const char *game_name, bool start_in_fullscreen) {
    videoSetMode(MODE_0_3D);
    consoleDemoInit();
}

static void gfx_nds_set_keyboard_callbacks(bool (*on_key_down)(int scancode), bool (*on_key_up)(int scancode), void (*on_all_keys_up)(void)) {
}

static void gfx_nds_set_fullscreen_changed_callback(void (*on_fullscreen_changed)(bool is_now_fullscreen)) {
}

static void gfx_nds_set_fullscreen(bool enable) {
}

static void gfx_nds_main_loop(void (*run_one_game_iter)(void)) {
    while (1) {
        run_one_game_iter();
    }
}

static void gfx_nds_get_dimensions(uint32_t *width, uint32_t *height) {
    *width = 256;
    *height = 192;
}

static void gfx_nds_handle_events(void) {
}

static bool gfx_nds_start_frame(void) {
    return true;
}

static void gfx_nds_swap_buffers_begin(void) {
}

static void gfx_nds_swap_buffers_end(void) {
    //swiWaitForVBlank();
}

static double gfx_nds_get_time(void) {
    return 0.0;
}

struct GfxWindowManagerAPI gfx_nds = {
    gfx_nds_init,
    gfx_nds_set_keyboard_callbacks,
    gfx_nds_set_fullscreen_changed_callback,
    gfx_nds_set_fullscreen,
    gfx_nds_main_loop,
    gfx_nds_get_dimensions,
    gfx_nds_handle_events,
    gfx_nds_start_frame,
    gfx_nds_swap_buffers_begin,
    gfx_nds_swap_buffers_end,
    gfx_nds_get_time
};

#endif
