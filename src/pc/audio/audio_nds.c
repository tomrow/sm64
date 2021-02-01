#ifdef TARGET_NDS

#include <nds.h>

#include "audio_api.h"

static bool audio_nds_init(void) {
    return true;
}

static int audio_nds_buffered(void) {
    return 0;
}

static int audio_nds_get_desired_buffered(void) {
    return 0;
}

static void audio_nds_play(const uint8_t *buf, size_t len) {
}

struct AudioAPI audio_nds = {
    audio_nds_init,
    audio_nds_buffered,
    audio_nds_get_desired_buffered,
    audio_nds_play
};

#endif
