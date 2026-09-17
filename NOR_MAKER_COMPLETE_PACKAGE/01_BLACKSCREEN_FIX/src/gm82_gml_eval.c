#define _POSIX_C_SOURCE 200809L
#include "gm82_gml_eval.h"
#include "gm82_gml_builtins.h"
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
    if (strcmp(name, "id") == 0) { *out = s ? (double)s->id : 0; return true; }
    if (strcmp(name, "object_index") == 0) { *out = s ? (double)s->object_index : 0; return true; }
    if (strcmp(name, "score") == 0) { *out = gml_get_score(); return true; }
    if (strcmp(name, "lives") == 0) { *out = gml_get_lives(); return true; }
    if (strcmp(name, "health") == 0) { *out = gml_get_health(); return true; }
    if (strcmp(name, "room") == 0) { *out = p->rt ? (double)p->rt->current_room : 0; return true; }
    if (strcmp(name, "room_width") == 0) { *out = p->rt ? (double)p->rt->room_width : 0; return true; }
    if (strcmp(name, "room_height") == 0) { *out = p->rt ? (double)p->rt->room_height : 0; return true; }
    if (strcmp(name, "room_speed") == 0) { *out = p->rt ? (double)p->rt->room_speed : 30; return true; }
    if (strcmp(name, "mouse_x") == 0) { *out = gml_mouse_x(); return true; }
    if (strcmp(name, "mouse_y") == 0) { *out = gml_mouse_y(); return true; }
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
    if (strcmp(name, "score") == 0) { gml_set_score(v); return true; }
    if (strcmp(name, "lives") == 0) { gml_set_lives(v); return true; }
    if (strcmp(name, "health") == 0) { gml_set_health(v); return true; }
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
        double arg = 0;
        if (peek(p) != ')') {
            if (!parse_expr(p, &arg)) return false;
        }
        if (!match(p, ')')) return false;
        if (strcmp(id, "abs") == 0) { *out = fabs(arg); return true; }
        if (strcmp(id, "sign") == 0) { *out = arg > 0 ? 1 : (arg < 0 ? -1 : 0); return true; }
        if (strcmp(id, "irandom") == 0) { *out = (double)(rand() % ((int)arg + 1)); return true; }
        if (strcmp(id, "floor") == 0) { *out = floor(arg); return true; }
        if (strcmp(id, "ceil") == 0) { *out = ceil(arg); return true; }
        if (strcmp(id, "round") == 0) { *out = round(arg); return true; }
        if (strcmp(id, "keyboard_check") == 0) {
            *out = gml_keyboard_check(arg); return true;
        }
        if (strcmp(id, "mouse_check_button") == 0) {
            *out = gml_mouse_check_button(arg); return true;
        }
        if (strcmp(id, "keyboard_check_pressed") == 0) {
            *out = gml_keyboard_check_pressed(arg); return true;
        }
        if (strcmp(id, "place_free") == 0) {
            /* place_free(x) only one arg – use self y */
            *out = gml_place_free(arg, p->self ? p->self->y : 0); return true;
        }
        if (strcmp(id, "instance_number") == 0) {
            *out = gml_instance_number(arg); return true;
        }
        if (strcmp(id, "irandom") == 0) { *out = (double)(rand() % ((int)arg + 1)); return true; }
        snprintf(p->err, sizeof(p->err), "unknown fn %s", id);
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
