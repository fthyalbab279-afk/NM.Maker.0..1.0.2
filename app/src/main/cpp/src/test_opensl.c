#include "gm82_sound_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main() {
    printf("=== Testing OpenSL ES Native Audio Device Driver ===\n");
    gm82_audio_driver drv = {0};
    gm82_opensl_init(&drv);

    uint8_t dummy_pcm[1024];
    for (int i = 0; i < 1024; i++) dummy_pcm[i] = (uint8_t)(i % 256);

    int submitted = gm82_opensl_submit_buffer(&drv, dummy_pcm, sizeof(dummy_pcm));

    printf("drv_initialized=%d | submitted=%d | total_submitted_bytes=%zu\n",
           drv.initialized, submitted, drv.buffer_submitted_bytes);

    if (drv.initialized && submitted && drv.buffer_submitted_bytes == 1024) {
        printf("OPENSL_AUDIO_TEST_PASS: opensl_init and pcm_buffer_submit verified successfully\n");
        return 0;
    } else {
        printf("OPENSL_AUDIO_TEST_FAIL: opensl_init or pcm_buffer_submit failed\n");
        return 1;
    }
}
