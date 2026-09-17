#ifndef GM82_INPUT_H
#define GM82_INPUT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Keyboard key codes – subset matching GM vk_ constants where possible */
enum {
    GM82_VK_NOKEY = 0,
    GM82_VK_LEFT = 37,
    GM82_VK_RIGHT = 39,
    GM82_VK_UP = 38,
    GM82_VK_DOWN = 40,
    GM82_VK_ENTER = 13,
    GM82_VK_SPACE = 32,
    GM82_VK_SHIFT = 16,
    GM82_VK_CONTROL = 17,
    GM82_VK_ESCAPE = 27,
    GM82_VK_A = 65,
    GM82_VK_D = 68,
    GM82_VK_W = 87,
    GM82_VK_S = 83
};

#define GM82_KEY_MAX 256

typedef struct {
    uint8_t down[GM82_KEY_MAX];      /* currently held */
    uint8_t pressed[GM82_KEY_MAX];   /* just pressed this frame */
    uint8_t released[GM82_KEY_MAX];  /* just released this frame */
    int32_t mouse_x, mouse_y;
    uint8_t mouse_button[3];         /* 0=left 1=right 2=middle */
} gm82_input_state;

void gm82_input_init(gm82_input_state *in);
void gm82_input_begin_frame(gm82_input_state *in); /* clears pressed/released */
void gm82_input_key_down(gm82_input_state *in, int key);
void gm82_input_key_up(gm82_input_state *in, int key);
void gm82_input_set_mouse(gm82_input_state *in, int x, int y);

/* GML-style queries */
double gml_keyboard_check(double key);
double gml_keyboard_check_pressed(double key);
double gml_keyboard_check_released(double key);
double gml_mouse_x(void);
double gml_mouse_y(void);
double gml_mouse_check_button(double button);

void gm82_input_bind_global(gm82_input_state *in);

#ifdef __cplusplus
}
#endif

#endif
