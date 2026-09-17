#ifndef GM82_RUNTIME_H
#define GM82_RUNTIME_H

#include "gm82_object_room_decode.h"
#include "gm82_sprite_decode.h"
/* sprite groups via gm82_sprite_group_list */
#include "gm82_background_decode.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gm82_action_table gm82_action_table;

#define GM82_MAX_INSTANCES 2048

typedef struct {
    int32_t id;
    int32_t object_index;
    int32_t sprite_index;
    double  x, y;
    double  hspeed, vspeed;
    double  direction, speed;
    double  gravity, gravity_direction;
    double  friction;
    int32_t image_index;
    double  image_speed;
    double  image_xscale;
    double  image_yscale;
    int32_t depth;
    int32_t solid;
    int32_t visible;
    int32_t persistent;
    int32_t alive;          /* 0 = destroyed */
    int32_t alarms[12];
    int32_t path_index;
    double  path_position;
    double  path_speed;
    int32_t timeline_index;
    double  timeline_position;
    double  timeline_speed;
    int32_t timeline_running;
} gm82_instance;

typedef struct {
    gm82_instance instances[GM82_MAX_INSTANCES];
    int32_t       instance_count;
    int32_t       next_id;

    int32_t room_width, room_height;
    int32_t room_speed;
    int32_t current_room;

    /* borrowed refs (not owned) */
    const gm82_decoded_object_list     *objects;
    const gm82_decoded_sprite_list     *sprites;
    const gm82_sprite_group_list       *sprite_groups;
    const gm82_decoded_background_list *backgrounds;
    const gm82_decoded_room_list       *rooms;
    const gm82_action_table            *actions;

    int32_t frame;
    int32_t running;
    /* Simple camera (subset of GM views) */
    int32_t view_enabled;
    double  view_x, view_y, view_w, view_h;
    int32_t view_follow_object; /* -1 none */
} gm82_runtime;

void gm82_runtime_init(gm82_runtime *rt);
void gm82_runtime_bind_assets(gm82_runtime *rt,
    const gm82_decoded_object_list *objects,
    const gm82_decoded_sprite_list *sprites,
    const gm82_decoded_background_list *backgrounds,
    const gm82_decoded_room_list *rooms,
    const gm82_action_table *actions);
void gm82_runtime_bind_sprite_groups(gm82_runtime *rt, const gm82_sprite_group_list *groups);
/* Fire user event 0..11 on all instances (or self if provided via events later) */
void gm82_runtime_event_user(gm82_runtime *rt, int user_event_index);

/* Load room by index: clear non-persistent instances, spawn from room data, fire Create */
bool gm82_runtime_goto_room(gm82_runtime *rt, int room_index);

/* One frame: alarms → begin step → step → end step */
void gm82_runtime_step(gm82_runtime *rt);

/* Software draw into RGBA buffer (caller owns buffer, size = room_w * room_h * 4) */
bool gm82_runtime_draw(gm82_runtime *rt, uint8_t *rgba, int32_t buf_w, int32_t buf_h);

gm82_instance *gm82_runtime_instance_create(gm82_runtime *rt, int32_t object_index, double x, double y);
void gm82_runtime_instance_destroy(gm82_runtime *rt, gm82_instance *inst);

#ifdef __cplusplus
}
#endif

#endif
