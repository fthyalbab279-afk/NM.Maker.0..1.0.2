#ifndef GM82_GML_EVAL_H
#define GM82_GML_EVAL_H

/*
 * Minimal GML expression / statement evaluator.
 * Supports: numbers, + - * /, parentheses, variables (x,y,hspeed,vspeed,speed,
 * direction, image_index, image_speed, solid, score, lives, health),
 * assignment, and a few function calls (abs, sign, irandom).
 *
 * NOT a full GML interpreter – no scripts, no with(), no arrays, no strings.
 */

#include "gm82_runtime.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Evaluate expression, result in *out. Returns false on parse error. */
bool gm82_gml_eval_expr(gm82_runtime *rt, gm82_instance *self,
                          const char *expr, double *out);

/* Execute one simple statement: "x = 10", "hspeed = hspeed + 1", "image_index = 0" */
bool gm82_gml_eval_stmt(gm82_runtime *rt, gm82_instance *self, const char *stmt);

/* Execute multiple statements separated by ; or newlines */
int gm82_gml_eval_block(gm82_runtime *rt, gm82_instance *self, const char *code);

#ifdef __cplusplus
}
#endif

#endif
