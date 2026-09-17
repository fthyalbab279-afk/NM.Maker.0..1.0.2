#ifndef GM82_PATH_H
#define GM82_PATH_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GM82_PATH_MAX_POINTS 64
#define GM82_PATH_MAX 16

typedef struct {
    double x, y;
    double speed; /* relative speed factor at point */
} gm82_path_point;

typedef struct {
    char name[64];
    gm82_path_point points[GM82_PATH_MAX_POINTS];
    int count;
    int closed;
} gm82_path;

typedef struct {
    gm82_path items[GM82_PATH_MAX];
    int count;
} gm82_path_list;

void gm82_path_list_init(gm82_path_list *L);
int  gm82_path_add(gm82_path_list *L, const char *name, int closed);
int  gm82_path_add_point(gm82_path_list *L, int path_index, double x, double y, double speed);
/* Advance position along path; returns 1 if still on path */
int  gm82_path_advance(const gm82_path *path, double *x, double *y,
                       double *pos /* 0..count */, double step);

#ifdef __cplusplus
}
#endif

#endif
