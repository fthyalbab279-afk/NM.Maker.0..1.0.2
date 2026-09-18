#define _POSIX_C_SOURCE 200809L
#include "gm82_gml_eval.h"
#include "gm82_gml_builtins.h"
#include "gm82_input.h"
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
    if (strcmp(name, "image_xscale") == 0) { *out = s ? s->image_xscale : 1; return true; }
    if (strcmp(name, "image_yscale") == 0) { *out = s ? s->image_yscale : 1; return true; }
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
    /* vk_ constants (GM key codes) */
    if (strcmp(name, "vk_left") == 0) { *out = 37; return true; }
    if (strcmp(name, "vk_right") == 0) { *out = 39; return true; }
    if (strcmp(name, "vk_up") == 0) { *out = 38; return true; }
    if (strcmp(name, "vk_down") == 0) { *out = 40; return true; }
    if (strcmp(name, "vk_enter") == 0) { *out = 13; return true; }
    if (strcmp(name, "vk_space") == 0) { *out = 32; return true; }
    if (strcmp(name, "vk_shift") == 0) { *out = 16; return true; }
    if (strcmp(name, "vk_control") == 0) { *out = 17; return true; }
    if (strcmp(name, "vk_escape") == 0) { *out = 27; return true; }
    if (strcmp(name, "vk_nokey") == 0) { *out = 0; return true; }
    if (strcmp(name, "vk_anykey") == 0) { *out = 1; return true; }
    /* Resource name → index (GM style: sprite_index = mini_mario) */
    if (p->rt) {
        if (p->rt->sprite_groups) {
            for (int i = 0; i < p->rt->sprite_groups->count; i++) {
                if (strcmp(p->rt->sprite_groups->items[i].name, name) == 0) {
                    *out = (double)i; return true;
                }
            }
        }
        if (p->rt->sprites) {
            for (int i = 0; i < p->rt->sprites->count; i++) {
                if (p->rt->sprites->frames[i].name[0] &&
                    strcmp(p->rt->sprites->frames[i].name, name) == 0) {
                    *out = (double)i; return true;
                }
            }
        }
        if (p->rt->objects) {
            for (int i = 0; i < p->rt->objects->count; i++) {
                if (strcmp(p->rt->objects->items[i].name, name) == 0) {
                    *out = (double)i; return true;
                }
            }
        }
    }
    /* unknown identifier: treat as 0 so assignment chains don't abort whole block */
    *out = 0;
    return true;
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
    if (strcmp(name, "image_xscale") == 0) { s->image_xscale = v; return true; }
    if (strcmp(name, "image_yscale") == 0) { s->image_yscale = v; return true; }
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
    if (c == '!') {
        p->i++;
        double v;
        if (!parse_primary(p, &v)) return false;
        *out = (v == 0) ? 1 : 0;
        return true;
    }
    char id[64];
    if (!parse_ident(p, id, sizeof(id))) return false;
    if (strcmp(id, "not") == 0) {
        double v;
        if (!parse_primary(p, &v)) return false;
        *out = (v == 0) ? 1 : 0;
        return true;
    }
    /* function call? collect up to 4 args */
    if (match(p, '(')) {
        double args[4] = {0,0,0,0};
        int nargs = 0;
        if (peek(p) != ')') {
            for (;;) {
                if (nargs >= 4) return false;
                if (!parse_expr(p, &args[nargs])) return false;
                nargs++;
                if (peek(p) != ',') break;
                getc_(p);
            }
        }
        if (!match(p, ')')) return false;
        double arg = args[0];
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
        if (strcmp(id, "keyboard_check_released") == 0) {
            *out = gml_keyboard_check_released(arg); return true;
        }
        if (strcmp(id, "place_free") == 0) {
            double yarg = (nargs >= 2) ? args[1] : (p->self ? p->self->y : 0);
            *out = gml_place_free(arg, yarg); return true;
        }
        if (strcmp(id, "place_meeting") == 0) {
            double yarg = (nargs >= 2) ? args[1] : 0;
            double oarg = (nargs >= 3) ? args[2] : -1;
            *out = gml_place_meeting(arg, yarg, oarg); return true;
        }
        if (strcmp(id, "place_empty") == 0) {
            double yarg = (nargs >= 2) ? args[1] : (p->self ? p->self->y : 0);
            *out = gml_place_empty(arg, yarg); return true;
        }
        if (strcmp(id, "instance_number") == 0) {
            *out = gml_instance_number(arg); return true;
        }
        if (strcmp(id, "instance_exists") == 0) {
            *out = gml_instance_exists(arg); return true;
        }
        if (strcmp(id, "instance_create") == 0) {
            double xarg = arg;
            double yarg = (nargs >= 2) ? args[1] : 0;
            double oarg = (nargs >= 3) ? args[2] : 0;
            *out = gml_instance_create(xarg, yarg, oarg); return true;
        }
        if (strcmp(id, "draw_self") == 0) {
            if (p->self) gml_draw_sprite((double)p->self->sprite_index, p->self->x, p->self->y);
            *out = 1; return true;
        }
        if (strcmp(id, "gravedad") == 0) {
            /* user script in mario sample – apply simple gravity */
            if (p->self) { p->self->gravity = 0.4; p->self->gravity_direction = 270; }
            *out = 0; return true;
        }
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

static bool parse_comparison(gml_parser *p, double *out) {
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

static bool parse_expr(gml_parser *p, double *out) {
    if (!parse_comparison(p, out)) return false;
    for (;;) {
        skip_ws(p);
        int andop = 0;
        if (p->i + 1 < p->n && p->s[p->i] == '&' && p->s[p->i+1] == '&') { andop = 1; p->i += 2; }
        else if (p->i + 1 < p->n && p->s[p->i] == '|' && p->s[p->i+1] == '|') { andop = 2; p->i += 2; }
        else if (p->i + 3 <= p->n && p->s[p->i]=='a' && p->s[p->i+1]=='n' && p->s[p->i+2]=='d' &&
                 (p->i+3>=p->n || !isalnum((unsigned char)p->s[p->i+3]))) { andop = 1; p->i += 3; }
        else if (p->i + 2 <= p->n && p->s[p->i]=='o' && p->s[p->i+1]=='r' &&
                 (p->i+2>=p->n || !isalnum((unsigned char)p->s[p->i+2]))) { andop = 2; p->i += 2; }
        else break;
        double r;
        if (!parse_comparison(p, &r)) return false;
        if (andop == 1) *out = (*out != 0 && r != 0) ? 1 : 0;
        else *out = (*out != 0 || r != 0) ? 1 : 0;
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
    /* if (cond) body [else body] – supports single stmt or { block } */
    if (strcmp(id, "if") == 0) {
        double cond = 0;
        skip_ws(&p);
        /* GML allows: if (expr)  OR  if expr   e.g. if keyboard_check(vk_left) */
        if (peek(&p) == '(') {
            getc_(&p);
            if (!parse_expr(&p, &cond)) return false;
            if (!match(&p, ')')) return false;
        } else {
            if (!parse_expr(&p, &cond)) return false;
        }
        skip_ws(&p);
        const char *rest = p.s + p.i;
        char then_buf[512], else_buf[512];
        then_buf[0] = else_buf[0] = 0;
        if (*rest == '{') {
            int depth = 0; size_t k = 0; const char *q = rest;
            while (*q && k + 1 < sizeof(then_buf)) {
                if (*q == '{') depth++;
                else if (*q == '}') {
                    depth--;
                    if (depth == 0) { q++; break; }
                }
                then_buf[k++] = *q++;
            }
            then_buf[k] = 0;
            rest = q;
        } else {
            size_t k = 0;
            while (rest[k] && rest[k] != ';' && rest[k] != '\n' && k + 1 < sizeof(then_buf)) {
                if ((k == 0 || isspace((unsigned char)rest[k-1]) || rest[k-1]==')') &&
                    strncmp(rest + k, "else", 4) == 0 &&
                    (rest[k+4]==0 || isspace((unsigned char)rest[k+4]) || rest[k+4]=='{'))
                    break;
                then_buf[k] = rest[k]; k++;
            }
            then_buf[k] = 0;
            rest += k;
            if (*rest == ';') rest++;
        }
        while (*rest && isspace((unsigned char)*rest)) rest++;
        if (strncmp(rest, "else", 4) == 0 &&
            (rest[4]==0 || isspace((unsigned char)rest[4]) || rest[4]=='{')) {
            rest += 4;
            while (*rest && isspace((unsigned char)*rest)) rest++;
            if (*rest == '{') {
                int depth = 0; size_t k = 0;
                while (*rest && k + 1 < sizeof(else_buf)) {
                    if (*rest == '{') depth++;
                    else if (*rest == '}') {
                        depth--;
                        if (depth == 0) { rest++; break; }
                    }
                    else_buf[k++] = *rest++;
                }
                else_buf[k] = 0;
            } else {
                size_t k = 0;
                while (rest[k] && rest[k] != ';' && rest[k] != '\n' && k + 1 < sizeof(else_buf)) {
                    else_buf[k] = rest[k]; k++;
                }
                else_buf[k] = 0;
            }
        }
        if (cond != 0) {
            if (then_buf[0] == '{') return gm82_gml_eval_block(rt, self, then_buf) > 0;
            return gm82_gml_eval_stmt(rt, self, then_buf);
        }
        if (else_buf[0]) {
            if (else_buf[0] == '{') return gm82_gml_eval_block(rt, self, else_buf) > 0;
            return gm82_gml_eval_stmt(rt, self, else_buf);
        }
        return true;
    }

    /* with (obj/id) body */
    if (strcmp(id, "with") == 0) {
        double target_expr = 0;
        skip_ws(&p);
        if (peek(&p) == '(') {
            getc_(&p);
            if (!parse_expr(&p, &target_expr)) return false;
            if (!match(&p, ')')) return false;
        } else {
            if (!parse_expr(&p, &target_expr)) return false;
        }
        skip_ws(&p);
        const char *body = p.s + p.i;
        if (!rt) return true;
        int target_oi = (int)target_expr;
        for (int i = 0; i < rt->instance_count; i++) {
            gm82_instance *inst = &rt->instances[i];
            if (!inst->alive) continue;
            if (target_oi >= 0 && inst->object_index != target_oi && inst->id != target_oi) continue;
            if (*body == '{') gm82_gml_eval_block(rt, inst, body);
            else gm82_gml_eval_stmt(rt, inst, body);
        }
        return true;
    }

    skip_ws(&p);
    /* compound: += -= *= /= */
    char op = 0;
    if (p.i + 1 < p.n && (p.s[p.i]=='+'||p.s[p.i]=='-'||p.s[p.i]=='*'||p.s[p.i]=='/') && p.s[p.i+1]=='=') {
        op = p.s[p.i];
        p.i += 2;
    } else if (!match(&p, '=')) {
        p.i = 0;
        double v;
        return parse_expr(&p, &v);
    }
    double v;
    if (!parse_expr(&p, &v)) return false;
    if (op) {
        double cur = 0;
        get_var(&p, id, &cur);
        if (op=='+') v = cur + v;
        else if (op=='-') v = cur - v;
        else if (op=='*') v = cur * v;
        else if (op=='/') v = (v!=0) ? cur / v : 0;
    }
    return set_var(&p, id, v);
}

int gm82_gml_eval_block(gm82_runtime *rt, gm82_instance *self, const char *code) {
    if (!code) return 0;
    int ok = 0;
    char buf[2048];
    const char *p = code;
    while (*p) {
        while (*p && (*p=='\r' || isspace((unsigned char)*p) || *p == ';' || *p == '{' || *p == '}')) p++;
        if (!*p) break;
        if (p[0]=='/' && p[1]=='/') {
            while (*p && *p != '\n') p++;
            continue;
        }
        size_t j = 0;
        int depth = 0;
        int is_if = (strncmp(p, "if", 2) == 0 && !isalnum((unsigned char)p[2]) && p[2] != '_');
        while (*p && j + 1 < sizeof(buf)) {
            if (*p == '{') { depth++; buf[j++] = *p++; continue; }
            if (*p == '}') {
                if (depth > 0) {
                    depth--;
                    buf[j++] = *p++;
                    if (is_if && depth == 0) break;
                    continue;
                }
                break;
            }
            if (depth == 0 && *p == ';') break;
            if (depth == 0 && *p == '\n' && !is_if) break;
            if (depth == 0 && *p == '\n' && is_if) { p++; continue; }
            if (*p != '\r') buf[j++] = *p;
            p++;
        }
        buf[j] = 0;
        if (*p == ';' || *p == '\n') p++;
        while (j > 0 && isspace((unsigned char)buf[j-1])) buf[--j] = 0;
        if (j > 0 && gm82_gml_eval_stmt(rt, self, buf)) ok++;
    }
    return ok;
}
