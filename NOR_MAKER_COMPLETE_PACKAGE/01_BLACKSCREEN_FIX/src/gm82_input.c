#define _POSIX_C_SOURCE 200809L
#include "gm82_input.h"
#include <string.h>

static gm82_input_state *g_input = NULL;

void gm82_input_bind_global(gm82_input_state *in) { g_input = in; }

void gm82_input_init(gm82_input_state *in) {
    memset(in, 0, sizeof(*in));
}

void gm82_input_begin_frame(gm82_input_state *in) {
    if (!in) return;
    memset(in->pressed, 0, sizeof(in->pressed));
    memset(in->released, 0, sizeof(in->released));
}

void gm82_input_key_down(gm82_input_state *in, int key) {
    if (!in || key < 0 || key >= GM82_KEY_MAX) return;
    if (!in->down[key]) in->pressed[key] = 1;
    in->down[key] = 1;
}

void gm82_input_key_up(gm82_input_state *in, int key) {
    if (!in || key < 0 || key >= GM82_KEY_MAX) return;
    if (in->down[key]) in->released[key] = 1;
    in->down[key] = 0;
}

void gm82_input_set_mouse(gm82_input_state *in, int x, int y) {
    if (!in) return;
    in->mouse_x = x;
    in->mouse_y = y;
}

double gml_keyboard_check(double key) {
    int k = (int)key;
    if (!g_input || k < 0 || k >= GM82_KEY_MAX) return 0;
    return g_input->down[k] ? 1.0 : 0.0;
}

double gml_keyboard_check_pressed(double key) {
    int k = (int)key;
    if (!g_input || k < 0 || k >= GM82_KEY_MAX) return 0;
    return g_input->pressed[k] ? 1.0 : 0.0;
}

double gml_keyboard_check_released(double key) {
    int k = (int)key;
    if (!g_input || k < 0 || k >= GM82_KEY_MAX) return 0;
    return g_input->released[k] ? 1.0 : 0.0;
}

double gml_mouse_x(void) { return g_input ? (double)g_input->mouse_x : 0; }
double gml_mouse_y(void) { return g_input ? (double)g_input->mouse_y : 0; }

double gml_mouse_check_button(double button) {
    int b = (int)button;
    if (!g_input || b < 0 || b > 2) return 0;
    return g_input->mouse_button[b] ? 1.0 : 0.0;
}

double gml_mouse_check_button_pressed(double button) {
    /* pressed edge not tracked separately for mouse yet – approximate with down */
    return gml_mouse_check_button(button);
}
