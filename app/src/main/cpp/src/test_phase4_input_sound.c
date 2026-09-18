#define _POSIX_C_SOURCE 200809L
#include "gm82_runtime.h"
#include "gm82_input.h"
#include "gm82_sound_runtime.h"
#include "gm82_gml_builtins.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

int main(void) {
    /* Test Input Subsystem */
    gm82_input_state in;
    gm82_input_init(&in);
    gm82_input_bind_global(&in);

    assert(gml_keyboard_check(GM82_VK_LEFT) == 0);

    gm82_input_key_down(&in, GM82_VK_LEFT);
    assert(gml_keyboard_check(GM82_VK_LEFT) == 1);
    assert(gml_keyboard_check_pressed(GM82_VK_LEFT) == 1);

    gm82_input_begin_frame(&in);
    assert(gml_keyboard_check(GM82_VK_LEFT) == 1);
    assert(gml_keyboard_check_pressed(GM82_VK_LEFT) == 0);

    gm82_input_key_up(&in, GM82_VK_LEFT);
    assert(gml_keyboard_check(GM82_VK_LEFT) == 0);
    assert(gml_keyboard_check_released(GM82_VK_LEFT) == 1);

    /* Test Sound Runtime */
    gm82_sound_runtime sr;
    gm82_sound_runtime_init(&sr);
    gm82_sound_set_global(&sr);

    gm82_sound_play(&sr, 0, 0);
    assert(sr.play_count == 1);
    assert(sr.last_played_index == 0);

    puts("PHASE4_INPUT_SOUND_TESTS_PASS");
    return 0;
}
