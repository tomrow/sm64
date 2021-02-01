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

#include "controller_api.h"

static void controller_nds_init(void) {
}

static void controller_nds_read(OSContPad *pad) {
    scanKeys();
    uint16_t keys = keysHeld();

    if (keys & KEY_A) {
        pad->button |= A_BUTTON;
    }
    if (keys & KEY_B) {
        pad->button |= B_BUTTON;
    }
    if (keys & KEY_X) {
        pad->button |= U_CBUTTONS;
    }
    if (keys & KEY_Y) {
        pad->button |= D_CBUTTONS;
    }
    if (keys & KEY_START) {
        pad->button |= START_BUTTON;
    }
    if (keys & KEY_L) {
        pad->button |= R_TRIG;
    }
    if (keys & KEY_R) {
        pad->button |= Z_TRIG;
    }

    if (keys & KEY_UP) {
        pad->stick_y = 127;
    }
    if (keys & KEY_DOWN) {
        pad->stick_y = -128;
    }
    if (keys & KEY_LEFT) {
        pad->stick_x = -128;
    }
    if (keys & KEY_RIGHT) {
        pad->stick_x = 127;
    }
}

struct ControllerAPI controller_nds = {
    controller_nds_init,
    controller_nds_read
};

#endif
