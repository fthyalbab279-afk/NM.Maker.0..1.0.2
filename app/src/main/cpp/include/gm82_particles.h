#ifndef GM82_PARTICLES_H
#define GM82_PARTICLES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GM82_PART_MAX 256
#define GM82_PSYS_MAX 8
#define GM82_PTYPE_MAX 16

typedef struct {
    int alive;
    double x, y;
    double hspeed, vspeed;
    double life, life_max;
    double size;
    uint32_t color;
} gm82_particle;

typedef struct {
    int used;
    double gravity, gravity_dir;
} gm82_particle_type;

typedef struct {
    int used;
    gm82_particle parts[GM82_PART_MAX];
} gm82_particle_system;

typedef struct {
    gm82_particle_system systems[GM82_PSYS_MAX];
    gm82_particle_type types[GM82_PTYPE_MAX];
} gm82_particle_world;

void gm82_particles_init(gm82_particle_world *w);
int  gm82_part_system_create(gm82_particle_world *w);
void gm82_part_system_destroy(gm82_particle_world *w, int sys);
int  gm82_part_type_create(gm82_particle_world *w);
void gm82_part_type_color(gm82_particle_world *w, int type, uint32_t color);
void gm82_part_particles_create(gm82_particle_world *w, int sys, double x, double y, int type, int number);
void gm82_part_system_update(gm82_particle_world *w, int sys);
/* Draw particles into RGBA buffer */
void gm82_part_system_draw(gm82_particle_world *w, int sys, uint8_t *rgba, int32_t bw, int32_t bh);

#ifdef __cplusplus
}
#endif

#endif
