#define _POSIX_C_SOURCE 200809L
#include "gml_frontend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void gml_ast_free(gml_ast *ast) {
    if (!ast) return;
    if (ast->text) free(ast->text);
    if (ast->left) gml_ast_free(ast->left);
    if (ast->right) gml_ast_free(ast->right);
    if (ast->items) {
        for (size_t i = 0; i < ast->count; i++) {
            gml_ast_free(ast->items[i]);
        }
        free(ast->items);
    }
    free(ast);
}

static gml_ast *new_node(gml_ast_kind kind) {
    gml_ast *n = (gml_ast *)calloc(1, sizeof(gml_ast));
    if (n) n->kind = kind;
    return n;
}

static void add_child(gml_ast *parent, gml_ast *child) {
    if (!parent || !child) return;
    gml_ast **new_items = (gml_ast **)realloc(parent->items, sizeof(gml_ast *) * (parent->count + 1));
    if (!new_items) return;
    parent->items = new_items;
    parent->items[parent->count++] = child;
}

typedef struct {
    const char *src;
    size_t pos;
    size_t len;
    char err[160];
} parser_t;

static void skip_ws_comments(parser_t *p) {
    while (p->pos < p->len) {
        char c = p->src[p->pos];
        if (isspace((unsigned char)c)) {
            p->pos++;
        } else if (c == '/' && p->pos + 1 < p->len && p->src[p->pos + 1] == '/') {
            p->pos += 2;
            while (p->pos < p->len && p->src[p->pos] != '\n') p->pos++;
        } else if (c == '/' && p->pos + 1 < p->len && p->src[p->pos + 1] == '*') {
            p->pos += 2;
            while (p->pos + 1 < p->len && !(p->src[p->pos] == '*' && p->src[p->pos + 1] == '/')) p->pos++;
            if (p->pos + 1 < p->len) p->pos += 2;
        } else {
            break;
        }
    }
}

static char peek_char(parser_t *p) {
    skip_ws_comments(p);
    return p->pos < p->len ? p->src[p->pos] : 0;
}

static char get_char(parser_t *p) {
    skip_ws_comments(p);
    return p->pos < p->len ? p->src[p->pos++] : 0;
}

static int match_keyword(parser_t *p, const char *kw) {
    skip_ws_comments(p);
    size_t kwlen = strlen(kw);
    if (p->pos + kwlen <= p->len && strncmp(p->src + p->pos, kw, kwlen) == 0) {
        char next = p->pos + kwlen < p->len ? p->src[p->pos + kwlen] : 0;
        if (!isalnum((unsigned char)next) && next != '_') {
            p->pos += kwlen;
            return 1;
        }
    }
    return 0;
}

static gml_ast *parse_expr(parser_t *p);
static gml_ast *parse_stmt(parser_t *p);

static gml_ast *parse_primary(parser_t *p) {
    skip_ws_comments(p);
    char c = peek_char(p);
    if (!c) return NULL;

    /* Number literal */
    if (isdigit((unsigned char)c) || (c == '.' && p->pos + 1 < p->len && isdigit((unsigned char)p->src[p->pos + 1]))) {
        char *end = NULL;
        double val = strtod(p->src + p->pos, &end);
        p->pos = (size_t)(end - p->src);
        gml_ast *node = new_node(GML_AST_NUMBER);
        node->number = val;
        return node;
    }

    /* String literal */
    if (c == '"' || c == '\'') {
        char quote = get_char(p);
        size_t start = p->pos;
        while (p->pos < p->len && p->src[p->pos] != quote) p->pos++;
        size_t slen = p->pos - start;
        char *str = (char *)malloc(slen + 1);
        if (str) {
            memcpy(str, p->src + start, slen);
            str[slen] = 0;
        }
        if (p->pos < p->len) p->pos++; /* skip closing quote */
        gml_ast *node = new_node(GML_AST_STRING);
        node->text = str;
        return node;
    }

    /* Parentheses */
    if (c == '(') {
        get_char(p);
        gml_ast *e = parse_expr(p);
        if (peek_char(p) == ')') get_char(p);
        return e;
    }

    /* Identifier or function call or keyword unary */
    if (isalpha((unsigned char)c) || c == '_') {
        size_t start = p->pos;
        while (p->pos < p->len && (isalnum((unsigned char)p->src[p->pos]) || p->src[p->pos] == '_')) p->pos++;
        size_t idlen = p->pos - start;
        char *id = (char *)malloc(idlen + 1);
        if (id) {
            memcpy(id, p->src + start, idlen);
            id[idlen] = 0;
        }

        if (id && strcmp(id, "not") == 0) {
            free(id);
            gml_ast *sub = parse_primary(p);
            gml_ast *node = new_node(GML_AST_UNARY);
            node->op = GML_T_NOT;
            node->left = sub;
            return node;
        }

        /* Check function call */
        if (peek_char(p) == '(') {
            get_char(p);
            gml_ast *call = new_node(GML_AST_CALL);
            call->text = id;
            if (peek_char(p) != ')') {
                for (;;) {
                    gml_ast *arg = parse_expr(p);
                    if (arg) add_child(call, arg);
                    if (peek_char(p) == ',') {
                        get_char(p);
                    } else {
                        break;
                    }
                }
            }
            if (peek_char(p) == ')') get_char(p);
            return call;
        }

        gml_ast *node = new_node(GML_AST_NAME);
        node->text = id;
        return node;
    }

    /* Unary operators */
    if (c == '-' || c == '+' || c == '!') {
        get_char(p);
        gml_ast *sub = parse_primary(p);
        gml_ast *node = new_node(GML_AST_UNARY);
        node->op = (c == '!') ? GML_T_NOT : (c == '-' ? GML_T_MINUS : GML_T_NONE);
        node->left = sub;
        return node;
    }

    return NULL;
}

static gml_ast *parse_postfix(parser_t *p) {
    gml_ast *node = parse_primary(p);
    while (node) {
        char c = peek_char(p);
        if (c == '[') {
            get_char(p);
            gml_ast *idx = parse_expr(p);
            if (peek_char(p) == ']') get_char(p);
            gml_ast *arr = new_node(GML_AST_INDEX);
            arr->left = node;
            arr->right = idx;
            node = arr;
        } else if (c == '.') {
            get_char(p);
            skip_ws_comments(p);
            size_t start = p->pos;
            while (p->pos < p->len && (isalnum((unsigned char)p->src[p->pos]) || p->src[p->pos] == '_')) p->pos++;
            size_t mlen = p->pos - start;
            char *mem = (char *)malloc(mlen + 1);
            if (mem) {
                memcpy(mem, p->src + start, mlen);
                mem[mlen] = 0;
            }
            gml_ast *mem_node = new_node(GML_AST_MEMBER);
            mem_node->left = node;
            mem_node->text = mem;
            node = mem_node;
        } else {
            break;
        }
    }
    return node;
}

static gml_ast *parse_multiplicative(parser_t *p) {
    gml_ast *left = parse_postfix(p);
    while (left) {
        char c = peek_char(p);
        if (c == '*' || c == '/' || c == '%') {
            get_char(p);
            gml_ast *right = parse_postfix(p);
            gml_ast *bin = new_node(GML_AST_BINARY);
            bin->op = (c == '*') ? GML_T_STAR : ((c == '/') ? GML_T_SLASH : GML_T_PERCENT);
            bin->left = left;
            bin->right = right;
            left = bin;
        } else {
            break;
        }
    }
    return left;
}

static gml_ast *parse_additive(parser_t *p) {
    gml_ast *left = parse_multiplicative(p);
    while (left) {
        char c = peek_char(p);
        if (c == '+' || c == '-') {
            get_char(p);
            gml_ast *right = parse_multiplicative(p);
            gml_ast *bin = new_node(GML_AST_BINARY);
            bin->op = (c == '+') ? GML_T_PLUS : GML_T_MINUS;
            bin->left = left;
            bin->right = right;
            left = bin;
        } else {
            break;
        }
    }
    return left;
}

static gml_ast *parse_relational(parser_t *p) {
    gml_ast *left = parse_additive(p);
    while (left) {
        skip_ws_comments(p);
        gml_op op = GML_T_NONE;
        if (p->pos + 1 < p->len && p->src[p->pos] == '=' && p->src[p->pos + 1] == '=') { op = GML_T_EQ; p->pos += 2; }
        else if (p->pos + 1 < p->len && p->src[p->pos] == '!' && p->src[p->pos + 1] == '=') { op = GML_T_NE; p->pos += 2; }
        else if (p->pos + 1 < p->len && p->src[p->pos] == '<' && p->src[p->pos + 1] == '=') { op = GML_T_LE; p->pos += 2; }
        else if (p->pos + 1 < p->len && p->src[p->pos] == '>' && p->src[p->pos + 1] == '=') { op = GML_T_GE; p->pos += 2; }
        else if (p->src[p->pos] == '<') { op = GML_T_LT; p->pos++; }
        else if (p->src[p->pos] == '>') { op = GML_T_GT; p->pos++; }

        if (op != GML_T_NONE) {
            gml_ast *right = parse_additive(p);
            gml_ast *bin = new_node(GML_AST_BINARY);
            bin->op = op;
            bin->left = left;
            bin->right = right;
            left = bin;
        } else {
            break;
        }
    }
    return left;
}

static gml_ast *parse_logical(parser_t *p) {
    gml_ast *left = parse_relational(p);
    while (left) {
        skip_ws_comments(p);
        gml_op op = GML_T_NONE;
        if (p->pos + 1 < p->len && p->src[p->pos] == '&' && p->src[p->pos + 1] == '&') { op = GML_T_AND; p->pos += 2; }
        else if (p->pos + 1 < p->len && p->src[p->pos] == '|' && p->src[p->pos + 1] == '|') { op = GML_T_OR; p->pos += 2; }
        else if (match_keyword(p, "and")) { op = GML_T_AND; }
        else if (match_keyword(p, "or")) { op = GML_T_OR; }

        if (op != GML_T_NONE) {
            gml_ast *right = parse_relational(p);
            gml_ast *bin = new_node(GML_AST_BINARY);
            bin->op = op;
            bin->left = left;
            bin->right = right;
            left = bin;
        } else {
            break;
        }
    }
    return left;
}

static gml_ast *parse_expr(parser_t *p) {
    gml_ast *left = parse_logical(p);
    skip_ws_comments(p);
    /* Assignment expression check (e.g. x = 5, x += 1) */
    if (left && (p->pos < p->len)) {
        if (p->src[p->pos] == '=' && (p->pos + 1 >= p->len || p->src[p->pos + 1] != '=')) {
            p->pos++;
            gml_ast *right = parse_expr(p);
            gml_ast *assign = new_node(GML_AST_ASSIGN);
            assign->left = left;
            assign->right = right;
            return assign;
        }
        if (p->pos + 1 < p->len && p->src[p->pos + 1] == '=' &&
            (p->src[p->pos] == '+' || p->src[p->pos] == '-' || p->src[p->pos] == '*' || p->src[p->pos] == '/')) {
            char op_c = p->src[p->pos];
            p->pos += 2;
            gml_ast *right = parse_expr(p);
            gml_ast *bin = new_node(GML_AST_BINARY);
            bin->op = (op_c == '+') ? GML_T_PLUS : ((op_c == '-') ? GML_T_MINUS : ((op_c == '*') ? GML_T_STAR : GML_T_SLASH));
            bin->left = left;
            bin->right = right;

            gml_ast *assign = new_node(GML_AST_ASSIGN);
            assign->left = left;
            assign->right = bin;
            return assign;
        }
    }
    return left;
}

static gml_ast *parse_stmt(parser_t *p) {
    skip_ws_comments(p);
    char c = peek_char(p);
    if (!c) return NULL;

    /* Block */
    if (c == '{') {
        get_char(p);
        gml_ast *blk = new_node(GML_AST_BLOCK);
        while (peek_char(p) && peek_char(p) != '}') {
            gml_ast *s = parse_stmt(p);
            if (s) add_child(blk, s);
        }
        if (peek_char(p) == '}') get_char(p);
        return blk;
    }

    /* Keywords */
    if (match_keyword(p, "return")) {
        gml_ast *ret = new_node(GML_AST_RETURN);
        if (peek_char(p) != ';') {
            ret->left = parse_expr(p);
        }
        if (peek_char(p) == ';') get_char(p);
        return ret;
    }

    if (match_keyword(p, "exit")) {
        if (peek_char(p) == ';') get_char(p);
        return new_node(GML_AST_EXIT);
    }

    if (match_keyword(p, "break")) {
        if (peek_char(p) == ';') get_char(p);
        return new_node(GML_AST_BREAK);
    }

    if (match_keyword(p, "continue")) {
        if (peek_char(p) == ';') get_char(p);
        return new_node(GML_AST_CONTINUE);
    }

    if (match_keyword(p, "if")) {
        gml_ast *if_node = new_node(GML_AST_IF);
        if_node->left = parse_expr(p);
        if_node->right = parse_stmt(p);
        if (match_keyword(p, "else")) {
            gml_ast *else_stmt = parse_stmt(p);
            if (else_stmt) add_child(if_node, else_stmt);
        }
        return if_node;
    }

    if (match_keyword(p, "while")) {
        gml_ast *w_node = new_node(GML_AST_WHILE);
        w_node->left = parse_expr(p);
        w_node->right = parse_stmt(p);
        return w_node;
    }

    if (match_keyword(p, "do")) {
        gml_ast *do_node = new_node(GML_AST_DO_UNTIL);
        do_node->right = parse_stmt(p);
        match_keyword(p, "until");
        do_node->left = parse_expr(p);
        if (peek_char(p) == ';') get_char(p);
        return do_node;
    }

    if (match_keyword(p, "repeat")) {
        gml_ast *rep = new_node(GML_AST_REPEAT);
        rep->left = parse_expr(p);
        rep->right = parse_stmt(p);
        return rep;
    }

    if (match_keyword(p, "with")) {
        gml_ast *w = new_node(GML_AST_WITH);
        w->left = parse_expr(p);
        w->right = parse_stmt(p);
        return w;
    }

    if (match_keyword(p, "for")) {
        gml_ast *f = new_node(GML_AST_FOR);
        if (peek_char(p) == '(') get_char(p);
        gml_ast *init_stmt = parse_expr(p);
        if (peek_char(p) == ';') get_char(p);
        gml_ast *cond_expr = parse_expr(p);
        if (peek_char(p) == ';') get_char(p);
        gml_ast *post_stmt = parse_expr(p);
        if (peek_char(p) == ')') get_char(p);
        gml_ast *body = parse_stmt(p);

        add_child(f, init_stmt ? init_stmt : new_node(GML_AST_NONE));
        add_child(f, cond_expr ? cond_expr : new_node(GML_AST_NONE));
        add_child(f, post_stmt ? post_stmt : new_node(GML_AST_NONE));
        add_child(f, body ? body : new_node(GML_AST_NONE));
        return f;
    }

    if (match_keyword(p, "switch")) {
        gml_ast *sw = new_node(GML_AST_SWITCH);
        sw->left = parse_expr(p);
        if (peek_char(p) == '{') get_char(p);
        while (peek_char(p) && peek_char(p) != '}') {
            if (match_keyword(p, "case")) {
                gml_ast *sc = new_node(GML_AST_SWITCH_CASE);
                sc->left = parse_expr(p);
                if (peek_char(p) == ':') get_char(p);
                gml_ast *blk = new_node(GML_AST_BLOCK);
                while (peek_char(p) && peek_char(p) != '}' &&
                       !(p->pos + 4 <= p->len && strncmp(p->src + p->pos, "case", 4) == 0) &&
                       !(p->pos + 7 <= p->len && strncmp(p->src + p->pos, "default", 7) == 0)) {
                    gml_ast *st = parse_stmt(p);
                    if (st) add_child(blk, st);
                }
                sc->right = blk;
                add_child(sw, sc);
            } else if (match_keyword(p, "default")) {
                if (peek_char(p) == ':') get_char(p);
                gml_ast *sc = new_node(GML_AST_SWITCH_CASE);
                sc->left = NULL;
                gml_ast *blk = new_node(GML_AST_BLOCK);
                while (peek_char(p) && peek_char(p) != '}' &&
                       !(p->pos + 4 <= p->len && strncmp(p->src + p->pos, "case", 4) == 0)) {
                    gml_ast *st = parse_stmt(p);
                    if (st) add_child(blk, st);
                }
                sc->right = blk;
                add_child(sw, sc);
            } else {
                get_char(p);
            }
        }
        if (peek_char(p) == '}') get_char(p);
        return sw;
    }

    /* Expression statement */
    gml_ast *expr = parse_expr(p);
    if (expr) {
        if (peek_char(p) == ';') get_char(p);
        gml_ast *stmt = new_node(GML_AST_EXPR_STMT);
        stmt->left = expr;
        return stmt;
    }

    if (peek_char(p) == ';') {
        get_char(p);
        return new_node(GML_AST_NONE);
    }

    return NULL;
}

int gml_parse_program(const char *source, gml_ast **out_ast, char *err_buf, size_t err_buf_size) {
    if (!source || !out_ast) return 0;
    parser_t p = { source, 0, strlen(source), {0} };
    gml_ast *root = new_node(GML_AST_BLOCK);

    while (p.pos < p.len) {
        skip_ws_comments(&p);
        if (p.pos >= p.len) break;
        gml_ast *st = parse_stmt(&p);
        if (st) {
            add_child(root, st);
        } else {
            /* skip unexpected token to prevent infinite loop */
            p.pos++;
        }
    }

    *out_ast = root;
    if (err_buf && err_buf_size > 0) err_buf[0] = 0;
    return 1;
}
