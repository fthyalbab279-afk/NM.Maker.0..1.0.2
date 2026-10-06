#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "gm82_path.h"
#include "gm82_timeline.h"
#include "gm82_particles.h"
#include "gm82_mp_grid.h"
#include "gm82_runtime.h"

static void test_paths(void) {
    gm82_path_list plist;
    gm82_path_list_init(&plist);
    assert(plist.count == 0);

    int pidx = gm82_path_add(&plist, "path_patrol", 0);
    assert(pidx == 0);
    assert(plist.count == 1);

    gm82_path_add_point(&plist, pidx, 0.0, 0.0, 1.0);
    gm82_path_add_point(&plist, pidx, 100.0, 0.0, 1.0);
    gm82_path_add_point(&plist, pidx, 100.0, 100.0, 1.0);

    const gm82_path *path = &plist.items[pidx];
    assert(path->count == 3);

    double x = 0.0, y = 0.0, pos = 0.0;
    int res = gm82_path_advance(path, &x, &y, &pos, 10.0);
    assert(res == 1);
    assert(x > 0.0);
    printf("  [PASS] test_paths: pos=%.2f, x=%.2f, y=%.2f\n", pos, x, y);
}

static void test_timelines(void) {
    gm82_timeline_list tlist;
    gm82_timeline_list_init(&tlist);
    assert(tlist.count == 0);

    int tidx = gm82_timeline_add(&tlist, "tl_sequence");
    assert(tidx == 0);
    assert(tlist.count == 1);

    int m1 = gm82_timeline_add_moment(&tlist, tidx, 0, "x = x + 10");
    int m2 = gm82_timeline_add_moment(&tlist, tidx, 30, "y = y + 5");
    assert(m1 == 1);
    assert(m2 == 1);

    const gm82_timeline *tl = &tlist.items[tidx];
    assert(tl->count == 2);
    assert(tl->moments[0].step == 0);
    assert(tl->moments[1].step == 30);
    printf("  [PASS] test_timelines: moment_count=%d\n", tl->count);
}

static void test_particles(void) {
    gm82_particle_world pworld;
    gm82_particles_init(&pworld);

    int sys = gm82_part_system_create(&pworld);
    assert(sys >= 0);

    int ptype = gm82_part_type_create(&pworld);
    assert(ptype >= 0);

    gm82_part_particles_create(&pworld, sys, 50.0, 50.0, ptype, 10);
    gm82_part_system_update(&pworld, sys);

    uint8_t canvas[100 * 100 * 4];
    memset(canvas, 0, sizeof(canvas));
    gm82_part_system_draw(&pworld, sys, canvas, 100, 100);

    printf("  [PASS] test_particles: sys=%d, ptype=%d\n", sys, ptype);
}

static void test_mp_grid(void) {
    gm82_mp_grid_world gworld;
    gm82_mp_grid_world_init(&gworld);

    int grid_id = gm82_mp_grid_create(&gworld, 0, 0, 10, 10, 16, 16);
    assert(grid_id >= 0);

    /* Add obstacle at cell (2,0) */
    gm82_mp_grid_add_cell(&gworld, grid_id, 2, 0, 1);

    double ox[64], oy[64];
    int npts = gm82_mp_grid_path(&gworld, grid_id, 8.0, 8.0, 80.0, 8.0, ox, oy, 64, 0);
    assert(npts > 0);
    printf("  [PASS] test_mp_grid: path points=%d, start=(%.1f,%.1f) goal=(%.1f,%.1f)\n",
           npts, ox[0], oy[0], ox[npts-1], oy[npts-1]);
}

static void test_alarm_handling(void) {
    gm82_runtime rt;
    gm82_runtime_init(&rt);

    gm82_instance *inst = gm82_runtime_instance_create(&rt, 0, 10.0, 10.0);
    assert(inst != NULL);

    inst->alarms[0] = 2; /* alarm 0 fires after 2 steps */
    rt.running = 1;

    gm82_runtime_step(&rt);
    assert(inst->alarms[0] == 1);

    gm82_runtime_step(&rt);
    assert(inst->alarms[0] == -1); /* fired and reset to -1 */

    printf("  [PASS] test_alarm_handling: alarm[0] countdown and fire verified\n");
}

int main(void) {
    printf("=== Testing Paths, Timelines, Particles, MP Grid & Alarms ===\n");
    test_paths();
    test_timelines();
    test_particles();
    test_mp_grid();
    test_alarm_handling();
    printf("PATHS_PARTICLES_MPGRID_TEST_PASS\n");
    return 0;
}
