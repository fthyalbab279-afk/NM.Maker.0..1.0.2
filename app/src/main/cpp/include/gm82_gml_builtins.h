#ifndef GM82_GML_BUILTINS_H
#define GM82_GML_BUILTINS_H

/*
 * Minimal GML built-in surface bound to gm82_runtime.
 * Not a full interpreter – callable C functions matching common GML names.
 * Real GML bytecode/script execution comes later.
 */
#include "gm82_runtime.h"
#include <stdint.h>
#include "gm82_path.h"
#include "gm82_timeline.h"
#include "gm82_particles.h"
#include "gm82_mp_grid.h"
#include "gm82_script.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Thread-local-ish current runtime for builtins (set before scripts run) */
void gm82_gml_set_runtime(gm82_runtime *rt);
gm82_runtime *gm82_gml_get_runtime(void);

/* Set "self" instance for instance-scoped builtins */
void gm82_gml_set_self(gm82_instance *self);
gm82_instance *gm82_gml_get_self(void);

/* ---- Instance ---- */
double gml_instance_create(double x, double y, double object_index);
void   gml_instance_destroy(void); /* destroys self */
double gml_instance_number(double object_index); /* -1 = all */
double gml_instance_exists(double id_or_object);
double gml_instance_nearest(double x, double y, double object_index);
double gml_instance_find(double object_index, double n);
double gml_distance_to_object(double object_index);

/* ---- Motion ---- */
void   gml_motion_set(double dir, double spd);
void   gml_motion_add(double dir, double spd);
void   gml_move_towards_point(double tx, double ty, double sp);
double gml_point_distance(double x1, double y1, double x2, double y2);
double gml_point_direction(double x1, double y1, double x2, double y2);

/* ---- Collision (AABB via sprite size) ---- */
double gml_place_meeting(double x, double y, double object_index);
double gml_position_meeting(double x, double y, double object_index);
double gml_instance_place(double x, double y, double object_index); /* returns id or -4 */

/* ---- Drawing (records to soft framebuffer if set) ---- */
void gm82_draw_set_target(uint8_t *rgba, int32_t w, int32_t h);
double gml_draw_clear(double color);
double gml_point_in_rectangle(double px, double py, double x1, double y1, double x2, double y2);
double gml_collision_rectangle(double x1, double y1, double x2, double y2, double obj, double prec, double notme);
double gml_collision_circle(double xc, double yc, double rad, double obj, double prec, double notme);
double gml_collision_point(double x, double y, double obj, double prec, double notme);
double gml_place_free(double x, double y);
double gml_place_empty(double x, double y);
double gml_move_contact_solid(double dir, double maxdist);
double gml_sprite_get_width(double sprite);
double gml_sprite_get_height(double sprite);
double gml_sprite_get_number(double sprite);
double gml_sprite_exists(double sprite);
double gml_object_exists(double object_index);
double gml_object_get_sprite(double object_index);
double gml_object_get_solid(double object_index);
double gml_get_timer(void);
double gml_delta_time(void);
void gm82_gml_set_frame_time(double seconds);
void gml_draw_sprite(double sprite, double x, double y);
void gml_draw_set_color(double color);
void gml_draw_set_alpha(double alpha);
double gml_draw_get_alpha(void);
double gml_draw_get_color(void);
void gml_draw_rectangle(double x1, double y1, double x2, double y2, double outline);
void gml_draw_circle(double x, double y, double r, double outline);
void gml_draw_line(double x1, double y1, double x2, double y2);
void gml_draw_text(double x, double y, const char *str);
void gml_draw_text_color(double x, double y, const char *str, double c1, double c2, double c3, double c4);

/* Surfaces (CPU framebuffer targets) */
double gml_surface_create(double w, double h);
double gml_surface_free(double id);
double gml_surface_exists(double id);
double gml_surface_get_width(double id);
double gml_surface_get_height(double id);
double gml_surface_set_target(double id);
double gml_surface_reset_target(void);
double gml_draw_surface(double id, double x, double y);

void gm82_particles_bind(gm82_particle_world *w);
double gml_part_system_create(void);
double gml_part_system_destroy(double sys);
double gml_part_type_create(void);
double gml_part_particles_create(double sys, double x, double y, double type, double number);
double gml_part_system_update(double sys);

/* ds_list (subset) */
double gml_ds_list_create(void);
double gml_ds_list_destroy(double id);
double gml_ds_list_add(double id, double value);
double gml_ds_list_find_value(double id, double pos);
double gml_ds_list_size(double id);
double gml_ds_list_clear(double id);
double gml_ds_list_delete(double id, double pos);
double gml_ds_list_find_index(double id, double value);
double gml_ds_list_empty(double id);

/* ds_map (numeric keys only) */
double gml_ds_map_create(void);
double gml_ds_map_destroy(double id);
double gml_ds_map_add(double id, double key, double value);
double gml_ds_map_find_value(double id, double key);
double gml_ds_map_exists(double id, double key);
double gml_ds_map_size(double id);
double gml_ds_map_clear(double id);
double gml_ds_map_delete(double id, double key);

/* ds_stack */
double gml_ds_stack_create(void);
double gml_ds_stack_destroy(double id);
double gml_ds_stack_push(double id, double value);
double gml_ds_stack_pop(double id);
double gml_ds_stack_top(double id);
double gml_ds_stack_size(double id);
double gml_ds_stack_empty(double id);

/* ds_queue */
double gml_ds_queue_create(void);
double gml_ds_queue_destroy(double id);
double gml_ds_queue_enqueue(double id, double value);
double gml_ds_queue_dequeue(double id);
double gml_ds_queue_head(double id);
double gml_ds_queue_tail(double id);
double gml_ds_queue_size(double id);
double gml_ds_queue_empty(double id);
double gml_ds_queue_clear(double id);

/* ds_priority (numeric priority, higher first) */
double gml_ds_priority_create(void);
double gml_ds_priority_destroy(double id);
double gml_ds_priority_add(double id, double value, double priority);
double gml_ds_priority_delete_max(double id);
double gml_ds_priority_find_max(double id);
double gml_ds_priority_size(double id);
double gml_ds_priority_empty(double id);

void gm82_mp_grid_bind(gm82_mp_grid_world *w);
double gml_mp_grid_create(double left, double top, double hcells, double vcells, double cellw, double cellh);
double gml_mp_grid_destroy(double id);
double gml_mp_grid_clear_all(double id, double solid);
double gml_mp_grid_add_cell(double id, double cx, double cy, double solid);
/* returns number of path points found (path stored internally for last call) */
double gml_mp_grid_path(double id, double xstart, double ystart, double xgoal, double ygoal, double allowdiag);

/* buffer (subset – grow fixed, little-endian numbers) */
double gml_buffer_create(double size, double type, double alignment);
double gml_buffer_delete(double id);
double gml_buffer_write(double id, double type, double value);
double gml_buffer_read(double id, double type);
double gml_buffer_poke(double id, double offset, double type, double value);
double gml_buffer_peek(double id, double offset, double type);
double gml_buffer_seek(double id, double base, double offset);
double gml_buffer_tell(double id);
double gml_buffer_get_size(double id);
double gml_buffer_sizeof(double type);

/* ini files (simple key=value, one section optional) */
double gml_ini_open(const char *path);
double gml_ini_close(void);
double gml_ini_write_real(const char *section, const char *key, double value);
double gml_ini_read_real(const char *section, const char *key, double def);
double gml_ini_key_exists(const char *section, const char *key);

/* text files (subset) */
double gml_file_text_open_read(const char *path);
double gml_file_text_open_write(const char *path);
double gml_file_text_open_append(const char *path);
double gml_file_text_close(double id);
double gml_file_text_eof(double id);
double gml_file_text_write_string(double id, const char *str);
double gml_file_text_write_real(double id, double value);
double gml_file_text_writeln(double id);
double gml_file_text_read_real(double id);
double gml_file_exists(const char *path);
double gml_file_delete(const char *path);

double gml_date_current_datetime(void);
double gml_date_get_year(double datetime);
double gml_date_get_month(double datetime);
double gml_date_get_day(double datetime);
double gml_date_get_hour(double datetime);
double gml_date_get_minute(double datetime);
double gml_date_get_second(double datetime);
double gml_current_time(void);
double gml_current_year(void);
double gml_current_month(void);
double gml_current_day(void);
double gml_random(double x);
double gml_random_range(double x1, double x2);
double gml_irandom_range(double x1, double x2);
double gml_choose(double a, double b);
double gml_lengthdir_x(double len, double dir);
double gml_lengthdir_y(double len, double dir);
double gml_point_distance(double x1, double y1, double x2, double y2);
double gml_point_direction(double x1, double y1, double x2, double y2);
double gml_lerp(double a, double b, double amount);
double gml_clamp(double val, double minv, double maxv);
double gml_deg_to_rad(double deg);
double gml_rad_to_deg(double rad);
double gml_angle_difference(double dest, double src);

double gml_display_get_width(void);
double gml_display_get_height(void);
double gml_window_get_width(void);
double gml_window_get_height(void);
double gml_window_set_caption(const char *caption);
const char *gml_window_get_caption(void);
double gml_room_goto(double room_index);
double gml_room_goto_next(void);
double gml_room_goto_previous(void);
double gml_room_restart(void);
double gml_game_end(void);
double gml_game_restart(void);
void gm82_window_set_size(int w, int h);
double gml_game_has_ended(void);
void gm82_scripts_bind(gm82_script_list *scripts);
double gml_script_exists(double script_index);
double gml_script_execute(double script_index);
double gml_alarm_set(double index, double steps);
double gml_alarm_get(double index);
/* Set code to run when alarm[index] fires (simple GML line) */
double gml_alarm_set_script(double index, const char *code);
void gm82_alarm_fire_scripts(gm82_runtime *rt, gm82_instance *inst, int alarm_index);

/* Debug / string helpers (C-side; not full GML strings) */
void gm82_debug_log(const char *msg);
int  gm82_debug_log_count(void);
const char *gm82_debug_log_get(int index);
double gml_string_length(const char *s);
double gml_real(const char *s);
double gml_string_pos(const char *sub, const char *str);
double gml_string_char_at(const char *str, double index);
double gml_string_digits(const char *str, char *out, size_t out_sz);
double gml_string_lower(const char *str, char *out, size_t out_sz);
double gml_string_upper(const char *str, char *out, size_t out_sz);
double gml_array_length_1d(double array_id);
double gml_array_height_2d(double array_id);
double gml_mouse_x(void);
double gml_mouse_y(void);
double gml_mouse_check_button(double button);
double gml_mouse_check_button_pressed(double button);
double gml_instance_change(double object_index, double perform_events);
double gml_instance_copy(double perform_events);
double gml_instance_deactivate_all(double notme);
double gml_instance_activate_all(void);
void gm82_path_bind(gm82_path_list *paths);
double gml_path_start(double path_index, double speed, double end_action, double absolute);
double gml_path_end(void);
double gml_path_get_number(void);
void gm82_path_step_instance(gm82_instance *inst);
void gm82_timeline_bind(gm82_timeline_list *tls);
double gml_timeline_start(double timeline_index, double position, double step, double direction);
double gml_timeline_stop(void);
void gm82_timeline_step_instance(gm82_runtime *rt, gm82_instance *inst);
void gml_draw_sprite_ext(double sprite, double subimg, double x, double y,
                         double xscale, double yscale, double rot, double color, double alpha);

/* ---- Built-in variable access on self ---- */
double gml_get_x(void);
double gml_get_y(void);
double gml_get_bbox_left(void);
double gml_get_bbox_right(void);
double gml_get_bbox_top(void);
double gml_get_bbox_bottom(void);
void   gml_set_x(double v);
void   gml_set_y(double v);
double gml_get_hspeed(void);
double gml_get_vspeed(void);
void   gml_set_hspeed(double v);
void   gml_set_vspeed(double v);
double gml_get_direction(void);
void   gml_set_direction(double v);
double gml_get_speed(void);
void   gml_set_speed(double v);
double gml_get_sprite_index(void);
void   gml_set_sprite_index(double v);
double gml_get_image_index(void);
void   gml_set_image_index(double v);
double gml_get_solid(void);
double gml_get_visible(void);

/* ---- Room ---- */
double gml_room_width(void);
double gml_room_height(void);
double gml_room_speed(void);
double gml_room_goto(double room_index);
double gml_room(void);
double gml_room_restart(void);
double gml_room_previous(void);
double gml_room_next(void);
double gml_game_end(void);

/* Global game vars (GM-style) */
double gml_get_score(void);
void   gml_set_score(double v);
double gml_get_lives(void);
void   gml_set_lives(double v);
double gml_get_health(void);
void   gml_set_health(double v);
/* Audio stubs – no device playback yet */
double gml_sound_play(double sound_index);
double gml_sound_stop(double sound_index);
double gml_sound_isplaying(double sound_index);

/* ---- Math helpers common in GML ---- */
double gml_abs(double v);
double gml_sign(double v);
double gml_clamp(double v, double lo, double hi);
double gml_lerp(double a, double b, double t);
double gml_irandom(double n);
double gml_random(double n);

#ifdef __cplusplus
}
#endif

#endif
