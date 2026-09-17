#ifndef GM82_EVENTS_H
#define GM82_EVENTS_H

#include "gm82_runtime.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Event type IDs (Game Maker) */
enum {
    GM82_EV_CREATE = 0,
    GM82_EV_DESTROY = 1,
    GM82_EV_ALARM = 2,
    GM82_EV_STEP = 3,
    GM82_EV_COLLISION = 4,
    GM82_EV_KEYBOARD = 5,
    GM82_EV_MOUSE = 6,
    GM82_EV_OTHER = 7,
    GM82_EV_DRAW = 8,
    GM82_EV_KEYPRESS = 9,
    GM82_EV_KEYRELEASE = 10
};

/* Step sub-types */
enum {
    GM82_STEP_NORMAL = 0,
    GM82_STEP_BEGIN = 1,
    GM82_STEP_END = 2
};

/*
 * Default behaviors registered by object name when full DnD/GML
 * action lists are not yet fully decoded from the GMK.
 * This is honest scaffolding – not a claim of full action support.
 */
typedef void (*gm82_event_fn)(gm82_runtime *rt, gm82_instance *self);

typedef struct {
    const char *object_name_prefix; /* matched against object name */
    gm82_event_fn on_create;
    gm82_event_fn on_step;
} gm82_behavior;

void gm82_events_register_defaults(void);

/* Fire Create for all instances after room load (also called from goto_room) */
void gm82_events_fire_create_all(gm82_runtime *rt);

/* Fire Step (begin/normal/end simplified into one) for all alive instances */
void gm82_events_fire_step_all(gm82_runtime *rt);
void gm82_events_fire_collision(gm82_runtime *rt, gm82_instance *inst, gm82_instance *other);

/* Lookup behavior for object index */
const gm82_behavior *gm82_events_find_behavior(gm82_runtime *rt, int32_t object_index);

/* Bind GML source to run on Step for objects whose name starts with prefix.
 * Replaces C behavior step when both exist for that instance (GML preferred).
 * Max 16 bindings. code pointer must remain valid. */
void gm82_events_clear_gml_bindings(void);
bool gm82_events_bind_gml_step(const char *object_name_prefix, const char *code);
bool gm82_events_bind_gml_create(const char *object_name_prefix, const char *code);

#ifdef __cplusplus
}
#endif

#endif

/* Best-effort: scan GMK object resource blobs for embedded GML-like strings
 * and bind them as Create events (not full event-type decode). */
int gm82_events_autobind_gml_from_gmk(const uint8_t *data, size_t size);
