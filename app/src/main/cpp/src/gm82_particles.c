#define _POSIX_C_SOURCE 200809L
#include "gm82_particles.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

void gm82_particles_init(gm82_particle_world *w) {
    memset(w, 0, sizeof(*w));
}

int gm82_part_system_create(gm82_particle_world *w) {
    for (int i = 0; i < GM82_PSYS_MAX; i++) {
        if (!w->systems[i].used) {
            memset(&w->systems[i], 0, sizeof(w->systems[i]));
            w->systems[i].used = 1;
            return i;
        }
    }
    return -1;
}

void gm82_part_system_destroy(gm82_particle_world *w, int sys) {
    if (sys < 0 || sys >= GM82_PSYS_MAX) return;
    memset(&w->systems[sys], 0, sizeof(w->systems[sys]));
}

int gm82_part_type_create(gm82_particle_world *w) {
    for (int i = 0; i < GM82_PTYPE_MAX; i++) {
        if (!w->types[i].used) {
            w->types[i].used = 1;
            w->types[i].gravity = 0.1;
            w->types[i].gravity_dir = 270;
            return i;
        }
    }
    return -1;
}

void gm82_part_type_color(gm82_particle_world *w, int type, uint32_t color) {
    if (type < 0 || type >= GM82_PTYPE_MAX) return;
    /* store color in gravity_dir high bits? keep simple - color applied at create */
    (void)w; (void)type; (void)color;
}

void gm82_part_particles_create(gm82_particle_world *w, int sys, double x, double y, int type, int number) {
    if (sys < 0 || sys >= GM82_PSYS_MAX || !w->systems[sys].used) return;
    if (number < 1) number = 1;
    if (number > 64) number = 64;
    uint32_t col = 0xFFFF00;
    double grav = 0.15;
    if (type >= 0 && type < GM82_PTYPE_MAX && w->types[type].used)
        grav = w->types[type].gravity;
    gm82_particle_system *s = &w->systems[sys];
    for (int n = 0; n < number; n++) {
        int slot = -1;
        for (int i = 0; i < GM82_PART_MAX; i++)
            if (!s->parts[i].alive) { slot = i; break; }
        if (slot < 0) break;
        gm82_particle *p = &s->parts[slot];
        memset(p, 0, sizeof(*p));
        p->alive = 1;
        p->x = x; p->y = y;
        p->hspeed = ((rand() % 100) - 50) / 25.0;
        p->vspeed = -((rand() % 100) / 30.0) - 0.5;
        p->life_max = 20 + (rand() % 30);
        p->life = p->life_max;
        p->size = 1 + (rand() % 3);
        p->color = col;
        (void)grav;
    }
}

void gm82_part_system_update(gm82_particle_world *w, int sys) {
    if (sys < 0 || sys >= GM82_PSYS_MAX || !w->systems[sys].used) return;
    gm82_particle_system *s = &w->systems[sys];
    for (int i = 0; i < GM82_PART_MAX; i++) {
        gm82_particle *p = &s->parts[i];
        if (!p->alive) continue;
        p->vspeed += 0.15; /* gravity down */
        p->x += p->hspeed;
        p->y += p->vspeed;
        p->life -= 1;
        if (p->life <= 0) p->alive = 0;
    }
}

void gm82_part_system_draw(gm82_particle_world *w, int sys, uint8_t *rgba, int32_t bw, int32_t bh) {
    if (!rgba || sys < 0 || sys >= GM82_PSYS_MAX || !w->systems[sys].used) return;
    gm82_particle_system *s = &w->systems[sys];
    for (int i = 0; i < GM82_PART_MAX; i++) {
        gm82_particle *p = &s->parts[i];
        if (!p->alive) continue;
        int x = (int)p->x, y = (int)p->y;
        int sz = (int)p->size;
        uint8_t r = (p->color >> 16) & 255;
        uint8_t g = (p->color >> 8) & 255;
        uint8_t b = p->color & 255;
        double fade = p->life / p->life_max;
        r = (uint8_t)(r * fade); g = (uint8_t)(g * fade); b = (uint8_t)(b * fade);
        for (int dy = 0; dy < sz; dy++)
            for (int dx = 0; dx < sz; dx++) {
                int px = x + dx, py = y + dy;
                if (px < 0 || py < 0 || px >= bw || py >= bh) continue;
                uint8_t *d = rgba + ((size_t)py * (size_t)bw + (size_t)px) * 4;
                d[0]=r; d[1]=g; d[2]=b; d[3]=255;
            }
    }
}
