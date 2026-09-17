#define _POSIX_C_SOURCE 200809L
#include "gm82_gml_eval.h"
#include "gm82_gml_builtins.h"
#include "gm82_script.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

typedef struct {
    const char *s;
    size_t i, n;
    gm82_runtime *rt;
    gm82_instance *self;
    char err[64];
} gml_parser;

static void skip_ws(gml_parser *p) {
    while (p->i < p->n && isspace((unsigned char)p->s[p->i])) p->i++;
}

static int peek(gml_parser *p) {
    skip_ws(p);
    return p->i < p->n ? (unsigned char)p->s[p->i] : 0;
}

static int getc_(gml_parser *p) {
    skip_ws(p);
    return p->i < p->n ? (unsigned char)p->s[p->i++] : 0;
}

static bool match(gml_parser *p, char c) {
    if (peek(p) == c) { p->i++; return true; }
    return false;
}

static bool parse_expr(gml_parser *p, double *out);

static bool parse_ident(gml_parser *p, char *buf, size_t buflen) {
    skip_ws(p);
    if (p->i >= p->n || !isalpha((unsigned char)p->s[p->i])) return false;
    size_t j = 0;
    while (p->i < p->n && (isalnum((unsigned char)p->s[p->i]) || p->s[p->i]=='_')) {
        if (j + 1 < buflen) buf[j++] = p->s[p->i];
        p->i++;
    }
    buf[j] = 0;
    return j > 0;
}

static bool get_var(gml_parser *p, const char *name, double *out) {
    gm82_instance *s = p->self;
    if (strcmp(name, "x") == 0) { *out = s ? s->x : 0; return true; }
    if (strcmp(name, "y") == 0) { *out = s ? s->y : 0; return true; }
    if (strcmp(name, "hspeed") == 0) { *out = s ? s->hspeed : 0; return true; }
    if (strcmp(name, "vspeed") == 0) { *out = s ? s->vspeed : 0; return true; }
    if (strcmp(name, "speed") == 0) { *out = s ? s->speed : 0; return true; }
    if (strcmp(name, "direction") == 0) { *out = s ? s->direction : 0; return true; }
    if (strcmp(name, "image_index") == 0) { *out = s ? (double)s->image_index : 0; return true; }
    if (strcmp(name, "image_speed") == 0) { *out = s ? s->image_speed : 0; return true; }
    if (strcmp(name, "sprite_index") == 0) { *out = s ? (double)s->sprite_index : 0; return true; }
    if (strcmp(name, "solid") == 0) { *out = s && s->solid ? 1 : 0; return true; }
    if (strcmp(name, "visible") == 0) { *out = s && s->visible ? 1 : 0; return true; }
    if (strcmp(name, "persistent") == 0) { *out = s && s->persistent ? 1 : 0; return true; }
    if (strcmp(name, "depth") == 0) { *out = s ? (double)s->depth : 0; return true; }
    if (strcmp(name, "gravity") == 0) { *out = s ? s->gravity : 0; return true; }
    if (strcmp(name, "gravity_direction") == 0) { *out = s ? s->gravity_direction : 270; return true; }
    if (strcmp(name, "friction") == 0) { *out = s ? s->friction : 0; return true; }
    if (strcmp(name, "id") == 0) { *out = s ? (double)s->id : 0; return true; }
    if (strcmp(name, "object_index") == 0) { *out = s ? (double)s->object_index : 0; return true; }
    if (strncmp(name, "alarm_", 6) == 0) {
        int idx = atoi(name + 6);
        if (s && idx >= 0 && idx < 12) { *out = (double)s->alarms[idx]; return true; }
    }
    if (strcmp(name, "score") == 0) { *out = gml_get_score(); return true; }
    if (strcmp(name, "lives") == 0) { *out = gml_get_lives(); return true; }
    if (strcmp(name, "health") == 0) { *out = gml_get_health(); return true; }
    if (strcmp(name, "room") == 0) { *out = p->rt ? (double)p->rt->current_room : 0; return true; }
    if (strcmp(name, "room_width") == 0) { *out = p->rt ? (double)p->rt->room_width : 0; return true; }
    if (strcmp(name, "room_height") == 0) { *out = p->rt ? (double)p->rt->room_height : 0; return true; }
    if (strcmp(name, "room_speed") == 0) { *out = p->rt ? (double)p->rt->room_speed : 30; return true; }
    if (strcmp(name, "mouse_x") == 0) { *out = gml_mouse_x(); return true; }
    if (strcmp(name, "mouse_y") == 0) { *out = gml_mouse_y(); return true; }
    /* Check instance user variables */
    if (s) {
        for (int vi = 0; vi < s->var_count; vi++) {
            if (strcmp(s->var_names[vi], name) == 0) {
                *out = s->var_values[vi];
                return true;
            }
        }
        /* Default unset user variable is 0 */
        *out = 0;
        return true;
    }
    snprintf(p->err, sizeof(p->err), "unknown var %s", name);
    return false;
}

static bool set_var(gml_parser *p, const char *name, double v) {
    gm82_instance *s = p->self;
    if (!s && !(strcmp(name,"score")==0 || strcmp(name,"lives")==0 || strcmp(name,"health")==0))
        return false;
    if (strcmp(name, "x") == 0) { s->x = v; return true; }
    if (strcmp(name, "y") == 0) { s->y = v; return true; }
    if (strcmp(name, "hspeed") == 0) { s->hspeed = v; return true; }
    if (strcmp(name, "vspeed") == 0) { s->vspeed = v; return true; }
    if (strcmp(name, "speed") == 0) { s->speed = v; return true; }
    if (strcmp(name, "direction") == 0) { s->direction = v; return true; }
    if (strcmp(name, "image_index") == 0) { s->image_index = (int32_t)v; return true; }
    if (strcmp(name, "image_speed") == 0) { s->image_speed = v; return true; }
    if (strcmp(name, "sprite_index") == 0) { s->sprite_index = (int32_t)v; return true; }
    if (strcmp(name, "solid") == 0) { s->solid = v != 0; return true; }
    if (strcmp(name, "visible") == 0) { s->visible = v != 0; return true; }
    if (strcmp(name, "persistent") == 0) { s->persistent = v != 0; return true; }
    if (strcmp(name, "depth") == 0) { s->depth = (int32_t)v; return true; }
    if (strcmp(name, "gravity") == 0) { s->gravity = v; return true; }
    if (strcmp(name, "gravity_direction") == 0) { s->gravity_direction = v; return true; }
    if (strcmp(name, "friction") == 0) { s->friction = v; return true; }
    if (strncmp(name, "alarm_", 6) == 0) {
        int idx = atoi(name + 6);
        if (s && idx >= 0 && idx < 12) { s->alarms[idx] = (int32_t)v; return true; }
    }
    if (strcmp(name, "score") == 0) { gml_set_score(v); return true; }
    if (strcmp(name, "lives") == 0) { gml_set_lives(v); return true; }
    if (strcmp(name, "health") == 0) { gml_set_health(v); return true; }
    /* Store or update instance user variables */
    if (s) {
        for (int vi = 0; vi < s->var_count; vi++) {
            if (strcmp(s->var_names[vi], name) == 0) {
                s->var_values[vi] = v;
                return true;
            }
        }
        if (s->var_count < 64) {
            strncpy(s->var_names[s->var_count], name, 31);
            s->var_names[s->var_count][31] = 0;
            s->var_values[s->var_count] = v;
            s->var_count++;
            return true;
        }
    }
    snprintf(p->err, sizeof(p->err), "cannot set %s", name);
    return false;
}

static bool parse_primary(gml_parser *p, double *out) {
    skip_ws(p);
    if (p->i >= p->n) return false;
    char c = p->s[p->i];
    if (c == '(') {
        p->i++;
        if (!parse_expr(p, out)) return false;
        if (!match(p, ')')) return false;
        return true;
    }
    if (isdigit((unsigned char)c) || (c == '.' && p->i+1 < p->n && isdigit((unsigned char)p->s[p->i+1]))) {
        char *end = NULL;
        *out = strtod(p->s + p->i, &end);
        if (end == p->s + p->i) return false;
        p->i = (size_t)(end - p->s);
        return true;
    }
    if (c == '-' || c == '+') {
        p->i++;
        double v;
        if (!parse_primary(p, &v)) return false;
        *out = (c == '-') ? -v : v;
        return true;
    }
    char id[64];
    if (!parse_ident(p, id, sizeof(id))) return false;
    /* function call? */
    if (match(p, '(')) {
        double args[8] = {0};
        size_t argc = 0;
        skip_ws(p);
        if (peek(p) != ')') {
            for (;;) {
                if (argc < 8) {
                    if (!parse_expr(p, &args[argc])) return false;
                    argc++;
                } else {
                    double dummy;
                    if (!parse_expr(p, &dummy)) return false;
                }
                skip_ws(p);
                if (peek(p) == ',') {
                    getc_(p);
                    skip_ws(p);
                } else {
                    break;
                }
            }
        }
        if (!match(p, ')')) return false;
        double arg = (argc > 0) ? args[0] : 0;
        if (strcmp(id, "abs") == 0) { *out = fabs(arg); return true; }
        if (strcmp(id, "sign") == 0) { *out = arg > 0 ? 1 : (arg < 0 ? -1 : 0); return true; }
        if (strcmp(id, "irandom") == 0) { *out = (double)(rand() % ((int)arg + 1)); return true; }
        if (strcmp(id, "irandom_range") == 0) {
            int lo = (int)args[0], hi = (int)args[1];
            if (hi < lo) { int t = lo; lo = hi; hi = t; }
            int range = hi - lo + 1;
            *out = range > 0 ? (double)(lo + (rand() % range)) : (double)lo;
            return true;
        }
        if (strcmp(id, "random") == 0) { *out = ((double)rand() / (double)RAND_MAX) * arg; return true; }
        if (strcmp(id, "random_range") == 0) {
            double lo = args[0], hi = args[1];
            *out = lo + ((double)rand() / (double)RAND_MAX) * (hi - lo);
            return true;
        }
        if (strcmp(id, "floor") == 0) { *out = floor(arg); return true; }
        if (strcmp(id, "ceil") == 0) { *out = ceil(arg); return true; }
        if (strcmp(id, "round") == 0) { *out = round(arg); return true; }
        if (strcmp(id, "sqrt") == 0) { *out = sqrt(arg); return true; }
        if (strcmp(id, "sqr") == 0) { *out = arg * arg; return true; }
        if (strcmp(id, "sin") == 0) { *out = sin(arg); return true; }
        if (strcmp(id, "cos") == 0) { *out = cos(arg); return true; }
        if (strcmp(id, "min") == 0) {
            double m = (argc > 0) ? args[0] : 0;
            for (size_t k = 1; k < argc; k++) if (args[k] < m) m = args[k];
            *out = m; return true;
        }
        if (strcmp(id, "max") == 0) {
            double m = (argc > 0) ? args[0] : 0;
            for (size_t k = 1; k < argc; k++) if (args[k] > m) m = args[k];
            *out = m; return true;
        }
        if (strcmp(id, "clamp") == 0) {
            double v = args[0], lo = args[1], hi = args[2];
            *out = (v < lo) ? lo : ((v > hi) ? hi : v); return true;
        }
        if (strcmp(id, "point_distance") == 0) {
            *out = gml_point_distance(args[0], args[1], args[2], args[3]); return true;
        }
        if (strcmp(id, "point_direction") == 0) {
            *out = gml_point_direction(args[0], args[1], args[2], args[3]); return true;
        }
        if (strcmp(id, "lengthdir_x") == 0) {
            *out = gml_lengthdir_x(args[0], args[1]); return true;
        }
        if (strcmp(id, "lengthdir_y") == 0) {
            *out = gml_lengthdir_y(args[0], args[1]); return true;
        }
        if (strcmp(id, "place_meeting") == 0) {
            *out = gml_place_meeting(args[0], args[1], args[2]); return true;
        }
        if (strcmp(id, "position_meeting") == 0) {
            *out = gml_position_meeting(args[0], args[1], args[2]); return true;
        }
        if (strcmp(id, "instance_place") == 0) {
            *out = gml_instance_place(args[0], args[1], args[2]); return true;
        }
        if (strcmp(id, "place_free") == 0) {
            *out = gml_place_free(args[0], (argc >= 2) ? args[1] : (p->self ? p->self->y : 0));
            return true;
        }
        if (strcmp(id, "instance_number") == 0) {
            *out = gml_instance_number(arg); return true;
        }
        if (strcmp(id, "instance_exists") == 0) {
            *out = gml_instance_exists(arg); return true;
        }
        if (strcmp(id, "instance_create") == 0) {
            *out = gml_instance_create(args[0], args[1], args[2]); return true;
        }
        if (strcmp(id, "instance_destroy") == 0) {
            gml_instance_destroy(); *out = 1; return true;
        }
        if (strcmp(id, "keyboard_check") == 0) {
            *out = gml_keyboard_check(arg); return true;
        }
        if (strcmp(id, "keyboard_check_pressed") == 0) {
            *out = gml_keyboard_check_pressed(arg); return true;
        }
        if (strcmp(id, "keyboard_check_released") == 0) {
            *out = gml_keyboard_check_released(arg); return true;
        }
        if (strcmp(id, "mouse_check_button") == 0) {
            *out = gml_mouse_check_button(arg); return true;
        }
        if (strcmp(id, "mouse_check_button_pressed") == 0) {
            *out = gml_mouse_check_button_pressed(arg); return true;
        }
        if (strcmp(id, "sound_play") == 0 || strcmp(id, "audio_play_sound") == 0) {
            *out = gml_sound_play(arg); return true;
        }
        if (strcmp(id, "sound_loop") == 0) {
            *out = gml_sound_loop(arg); return true;
        }
        if (strcmp(id, "sound_stop") == 0) {
            *out = gml_sound_stop(arg); return true;
        }
        if (strcmp(id, "room_goto") == 0) {
            *out = gml_room_goto(arg); return true;
        }
        if (strcmp(id, "room_goto_next") == 0) {
            *out = gml_room_goto_next(); return true;
        }
        if (strcmp(id, "room_goto_previous") == 0) {
            *out = gml_room_goto_previous(); return true;
        }
        if (strcmp(id, "room_restart") == 0) {
            *out = gml_room_restart(); return true;
        }
        if (strcmp(id, "game_restart") == 0) {
            *out = gml_game_restart(); return true;
        }
        if (strcmp(id, "game_end") == 0) {
            *out = gml_game_end(); return true;
        }
        if (strcmp(id, "event_user") == 0) {
            gm82_runtime *rt = gm82_gml_get_runtime();
            if (rt) gm82_runtime_event_user(rt, (int)arg);
            *out = 1; return true;
        }
        if (strcmp(id, "make_color_rgb") == 0) {
            int r = (int)args[0] & 0xFF, g = (int)args[1] & 0xFF, b = (int)args[2] & 0xFF;
            *out = (double)(r | (g << 8) | (b << 16)); return true;
        }
        if (strcmp(id, "instance_find") == 0) {
            *out = gml_instance_find(args[0], args[1]); return true;
        }
        if (strcmp(id, "instance_nearest") == 0) {
            *out = gml_instance_nearest(args[0], args[1], args[2]); return true;
        }
        if (strcmp(id, "draw_sprite") == 0) {
            gml_draw_sprite(args[0], args[1], args[2]); *out = 1; return true;
        }
        if (strcmp(id, "draw_text") == 0) {
            *out = 1; return true;
        }
        if (strcmp(id, "draw_rectangle") == 0) {
            gml_draw_rectangle(args[0], args[1], args[2], args[3], (argc >= 5) ? args[4] : 0);
            *out = 1; return true;
        }
        if (strcmp(id, "draw_circle") == 0) {
            gml_draw_circle(args[0], args[1], args[2], (argc >= 4) ? args[3] : 0);
            *out = 1; return true;
        }
        if (strcmp(id, "draw_set_color") == 0) {
            gml_draw_set_color(arg); *out = 1; return true;
        }
        if (strcmp(id, "draw_set_alpha") == 0) {
            gml_draw_set_alpha(arg); *out = 1; return true;
        }
        if (strcmp(id, "sprite_get_width") == 0) {
            *out = gml_sprite_get_width(arg); return true;
        }
        if (strcmp(id, "sprite_get_height") == 0) {
            *out = gml_sprite_get_height(arg); return true;
        }
        /* Fallback: Check if this function name matches an embedded GML script */
        if (p->rt && p->rt->scripts) {
            const gm82_script_list *sl = (const gm82_script_list *)p->rt->scripts;
            int sidx = gm82_script_find(sl, id);
            if (sidx >= 0 && sl->items[sidx].code[0]) {
                /* Set script argument variables: argument0, argument1, ... */
                if (p->self) {
                    char arg_name[16];
                    for (size_t ai = 0; ai < argc && ai < 16; ai++) {
                        snprintf(arg_name, sizeof(arg_name), "argument%zu", ai);
                        set_var(p, arg_name, args[ai]);
                    }
                }
                gm82_gml_eval_block(p->rt, p->self, sl->items[sidx].code);
                *out = 1;
                return true;
            }
        }
        snprintf(p->err, sizeof(p->err), "unknown fn %s", id);
        return false;
    }
    /* array indexing: id[index] */
    if (match(p, '[')) {
        double idx = 0;
        if (parse_expr(p, &idx) && match(p, ']')) {
            char arr_var[80];
            snprintf(arr_var, sizeof(arr_var), "%s_%d", id, (int)idx);
            return get_var(p, arr_var, out);
        }
        return false;
    }
    return get_var(p, id, out);
}

static bool parse_term(gml_parser *p, double *out) {
    if (!parse_primary(p, out)) return false;
    for (;;) {
        char c = peek(p);
        if (c != '*' && c != '/') break;
        getc_(p);
        double r;
        if (!parse_primary(p, &r)) return false;
        if (c == '*') *out *= r;
        else *out = (r != 0) ? (*out / r) : 0;
    }
    return true;
}

static bool parse_additive(gml_parser *p, double *out) {
    if (!parse_term(p, out)) return false;
    for (;;) {
        char c = peek(p);
        if (c != '+' && c != '-') break;
        getc_(p);
        double r;
        if (!parse_term(p, &r)) return false;
        if (c == '+') *out += r;
        else *out -= r;
    }
    return true;
}

static bool parse_expr(gml_parser *p, double *out) {
    if (!parse_additive(p, out)) return false;
    for (;;) {
        skip_ws(p);
        int op = 0; /* 1=< 2=> 3=<= 4=>= 5=== 6=!= */
        if (p->i + 1 < p->n && p->s[p->i] == '<' && p->s[p->i+1] == '=') { op = 3; p->i += 2; }
        else if (p->i + 1 < p->n && p->s[p->i] == '>' && p->s[p->i+1] == '=') { op = 4; p->i += 2; }
        else if (p->i + 1 < p->n && p->s[p->i] == '=' && p->s[p->i+1] == '=') { op = 5; p->i += 2; }
        else if (p->i + 1 < p->n && p->s[p->i] == '!' && p->s[p->i+1] == '=') { op = 6; p->i += 2; }
        else if (peek(p) == '<') { op = 1; p->i++; }
        else if (peek(p) == '>') { op = 2; p->i++; }
        else break;
        double r;
        if (!parse_additive(p, &r)) return false;
        switch (op) {
            case 1: *out = (*out < r) ? 1 : 0; break;
            case 2: *out = (*out > r) ? 1 : 0; break;
            case 3: *out = (*out <= r) ? 1 : 0; break;
            case 4: *out = (*out >= r) ? 1 : 0; break;
            case 5: *out = (*out == r) ? 1 : 0; break;
            case 6: *out = (*out != r) ? 1 : 0; break;
        }
    }
    return true;
}

bool gm82_gml_eval_expr(gm82_runtime *rt, gm82_instance *self, const char *expr, double *out) {
    if (!expr || !out) return false;
    gml_parser p = { expr, 0, strlen(expr), rt, self, {0} };
    gm82_gml_set_runtime(rt);
    gm82_gml_set_self(self);
    return parse_expr(&p, out);
}

bool gm82_gml_eval_stmt(gm82_runtime *rt, gm82_instance *self, const char *stmt) {
    if (!stmt) return false;
    gml_parser p = { stmt, 0, strlen(stmt), rt, self, {0} };
    gm82_gml_set_runtime(rt);
    gm82_gml_set_self(self);
    char id[64];
    if (!parse_ident(&p, id, sizeof(id))) return false;
    /* if (cond) body */
    if (strcmp(id, "if") == 0) {
        if (!match(&p, '(')) return false;
        double cond = 0;
        if (!parse_expr(&p, &cond)) return false;
        if (!match(&p, ')')) return false;
        skip_ws(&p);
        /* split then/else by finding " else " at top level (simple) */
        const char *rest = p.s + p.i;
        const char *else_pos = strstr(rest, " else ");
        char then_buf[256], else_buf[256];
        if (else_pos) {
            size_t tl = (size_t)(else_pos - rest);
            if (tl >= sizeof(then_buf)) tl = sizeof(then_buf)-1;
            memcpy(then_buf, rest, tl); then_buf[tl] = 0;
            strncpy(else_buf, else_pos + 6, sizeof(else_buf)-1); else_buf[sizeof(else_buf)-1]=0;
            if (cond != 0) return gm82_gml_eval_stmt(rt, self, then_buf);
            return gm82_gml_eval_stmt(rt, self, else_buf);
        }
        if (cond == 0) return true;
        return gm82_gml_eval_stmt(rt, self, rest);
    }
    /* repeat (count) body */
    if (strcmp(id, "repeat") == 0) {
        if (!match(&p, '(')) return false;
        double count = 0;
        if (!parse_expr(&p, &count)) return false;
        if (!match(&p, ')')) return false;
        skip_ws(&p);
        int n = (int)count;
        const char *body = p.s + p.i;
        for (int ri = 0; ri < n; ri++) {
            gm82_gml_eval_stmt(rt, self, body);
        }
        return true;
    }
    /* with (target) body */
    if (strcmp(id, "with") == 0) {
        if (!match(&p, '(')) return false;
        char target_id[64];
        if (!parse_ident(&p, target_id, sizeof(target_id))) return false;
        if (!match(&p, ')')) return false;
        skip_ws(&p);
        const char *body = p.s + p.i;
        if (rt) {
            int32_t oi = -1;
            if (strcmp(target_id, "all") == 0) oi = -1;
            else if (strcmp(target_id, "other") == 0) {
                extern gm82_instance *gml_get_other(void);
                gm82_instance *other = gml_get_other();
                if (other && other->alive) gm82_gml_eval_stmt(rt, other, body);
                return true;
            }
            for (int i = 0; i < rt->instance_count; i++) {
                gm82_instance *inst = &rt->instances[i];
                if (!inst->alive) continue;
                if (oi == -1 || inst->object_index == oi) {
                    gm82_gml_eval_stmt(rt, inst, body);
                }
            }
        }
        return true;
    }
    /* array assignment: id[index] = expr */
    if (match(&p, '[')) {
        double idx = 0;
        if (parse_expr(&p, &idx) && match(&p, ']')) {
            char arr_var[80];
            snprintf(arr_var, sizeof(arr_var), "%s_%d", id, (int)idx);
            if (!match(&p, '=')) return false;
            double v;
            if (!parse_expr(&p, &v)) return false;
            return set_var(&p, arr_var, v);
        }
        return false;
    }
    if (!match(&p, '=')) {
        p.i = 0;
        double v;
        return parse_expr(&p, &v);
    }
    double v;
    if (!parse_expr(&p, &v)) return false;
    return set_var(&p, id, v);
}

int gm82_gml_eval_block(gm82_runtime *rt, gm82_instance *self, const char *code) {
    if (!code) return 0;
    int ok = 0;
    char buf[256];
    const char *p = code;
    while (*p) {
        while (*p && (isspace((unsigned char)*p) || *p == ';')) p++;
        if (!*p) break;
        size_t j = 0;
        while (*p && *p != ';' && *p != '\n' && j + 1 < sizeof(buf))
            buf[j++] = *p++;
        buf[j] = 0;
        if (j > 0 && gm82_gml_eval_stmt(rt, self, buf)) ok++;
    }
    return ok;
}
