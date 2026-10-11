#define _POSIX_C_SOURCE 200809L
#include "gml_frontend.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

static gml_ast *ast_new(gml_ast_kind kind) {
    gml_ast *n = (gml_ast *)calloc(1, sizeof(gml_ast));
    if (n) n->kind = kind;
    return n;
}

void gml_ast_free(gml_ast *ast) {
    if (!ast) return;
    free(ast->text);
    gml_ast_free(ast->left);
    gml_ast_free(ast->right);
    if (ast->items) {
        for (size_t i = 0; i < ast->count; i++) {
            gml_ast_free(ast->items[i]);
        }
        free(ast->items);
    }
    free(ast);
}

static void ast_add_child(gml_ast *parent, gml_ast *child) {
    if (!parent || !child) return;
    gml_ast **new_items = (gml_ast **)realloc(parent->items, (parent->count + 1) * sizeof(gml_ast *));
    if (new_items) {
        parent->items = new_items;
        parent->items[parent->count++] = child;
    }
}

typedef struct {
    const char *src;
    size_t pos, len;
    char error[160];
} gml_lexer;

static void skip_space_comments(gml_lexer *l) {
    while (l->pos < l->len) {
        char c = l->src[l->pos];
        if (isspace((unsigned char)c)) { l->pos++; continue; }
        if (c == '/' && l->pos + 1 < l->len && l->src[l->pos+1] == '/') {
            l->pos += 2;
            while (l->pos < l->len && l->src[l->pos] != '\n') l->pos++;
            continue;
        }
        if (c == '/' && l->pos + 1 < l->len && l->src[l->pos+1] == '*') {
            l->pos += 2;
            while (l->pos + 1 < l->len && !(l->src[l->pos] == '*' && l->src[l->pos+1] == '/')) l->pos++;
            if (l->pos + 1 < l->len) l->pos += 2;
            continue;
        }
        break;
    }
}

static gml_ast *parse_expression(gml_lexer *l);

static gml_ast *parse_primary_expr(gml_lexer *l) {
    skip_space_comments(l);
    if (l->pos >= l->len) return NULL;
    char c = l->src[l->pos];

    /* Number literal */
    if (isdigit((unsigned char)c) || (c == '.' && l->pos + 1 < l->len && isdigit((unsigned char)l->src[l->pos+1]))) {
        char *end = NULL;
        double val = strtod(l->src + l->pos, &end);
        l->pos = (size_t)(end - l->src);
        gml_ast *n = ast_new(GML_AST_NUMBER);
        if (n) n->number = val;
        return n;
    }

    /* String literal */
    if (c == '"' || c == '\'') {
        char quote = c;
        l->pos++;
        size_t start = l->pos;
        while (l->pos < l->len && l->src[l->pos] != quote) l->pos++;
        size_t slen = l->pos - start;
        if (l->pos < l->len) l->pos++;
        gml_ast *n = ast_new(GML_AST_STRING);
        if (n) {
            n->text = (char *)malloc(slen + 1);
            if (n->text) {
                memcpy(n->text, l->src + start, slen);
                n->text[slen] = 0;
            }
        }
        return n;
    }

    /* Parenthesized expression */
    if (c == '(') {
        l->pos++;
        gml_ast *sub = parse_expression(l);
        skip_space_comments(l);
        if (l->pos < l->len && l->src[l->pos] == ')') l->pos++;
        return sub;
    }

    /* Identifier or function call */
    if (isalpha((unsigned char)c) || c == '_') {
        size_t start = l->pos;
        while (l->pos < l->len && (isalnum((unsigned char)l->src[l->pos]) || l->src[l->pos] == '_')) l->pos++;
        size_t idlen = l->pos - start;
        char name[64];
        if (idlen >= sizeof(name)) idlen = sizeof(name) - 1;
        memcpy(name, l->src + start, idlen);
        name[idlen] = 0;

        skip_space_comments(l);
        if (l->pos < l->len && l->src[l->pos] == '(') {
            /* Function call */
            l->pos++;
            gml_ast *call_node = ast_new(GML_AST_CALL);
            if (call_node) {
                call_node->text = strdup(name);
                skip_space_comments(l);
                if (l->pos < l->len && l->src[l->pos] != ')') {
                    for (;;) {
                        gml_ast *arg = parse_expression(l);
                        if (arg) ast_add_child(call_node, arg);
                        skip_space_comments(l);
                        if (l->pos < l->len && l->src[l->pos] == ',') { l->pos++; continue; }
                        break;
                    }
                }
                if (l->pos < l->len && l->src[l->pos] == ')') l->pos++;
            }
            return call_node;
        }

        gml_ast *var_node = ast_new(GML_AST_NAME);
        if (var_node) var_node->text = strdup(name);
        return var_node;
    }

    return NULL;
}

static gml_ast *parse_binary_expr(gml_lexer *l, int min_prec) {
    gml_ast *lhs = parse_primary_expr(l);
    if (!lhs) return NULL;

    for (;;) {
        skip_space_comments(l);
        if (l->pos >= l->len) break;
        char op = l->src[l->pos];
        gml_op gop = GML_T_NONE;
        int prec = 0;

        if (op == '+' || op == '-') { gop = (op == '+') ? GML_T_PLUS : GML_T_MINUS; prec = 10; }
        else if (op == '*' || op == '/') { gop = (op == '*') ? GML_T_STAR : GML_T_SLASH; prec = 20; }
        else if (op == '%') { gop = GML_T_PERCENT; prec = 20; }
        else if (op == '=') {
            if (l->pos + 1 < l->len && l->src[l->pos+1] == '=') { gop = GML_T_EQ; l->pos++; prec = 5; }
            else { break; /* Assignment operator handled separately */ }
        }
        else if (op == '!' && l->pos + 1 < l->len && l->src[l->pos+1] == '=') { gop = GML_T_NE; l->pos++; prec = 5; }
        else if (op == '<') {
            if (l->pos + 1 < l->len && l->src[l->pos+1] == '=') { gop = GML_T_LE; l->pos++; prec = 5; }
            else { gop = GML_T_LT; prec = 5; }
        }
        else if (op == '>') {
            if (l->pos + 1 < l->len && l->src[l->pos+1] == '=') { gop = GML_T_GE; l->pos++; prec = 5; }
            else { gop = GML_T_GT; prec = 5; }
        }
        else break;

        if (prec < min_prec) break;
        l->pos++; /* Consume operator */

        gml_ast *rhs = parse_binary_expr(l, prec + 1);
        gml_ast *bin = ast_new(GML_AST_BINARY);
        if (bin) {
            bin->op = gop;
            bin->left = lhs;
            bin->right = rhs;
            lhs = bin;
        }
    }
    return lhs;
}

static gml_ast *parse_expression(gml_lexer *l) {
    gml_ast *lhs = parse_binary_expr(l, 0);
    if (!lhs) return NULL;

    skip_space_comments(l);
    if (l->pos < l->len && l->src[l->pos] == '=') {
        /* Assignment */
        l->pos++;
        gml_ast *rhs = parse_expression(l);
        gml_ast *assign = ast_new(GML_AST_ASSIGN);
        if (assign) {
            assign->left = lhs;
            assign->right = rhs;
            return assign;
        }
    }
    return lhs;
}

static gml_ast *parse_statement(gml_lexer *l) {
    skip_space_comments(l);
    if (l->pos >= l->len) return NULL;

    /* Check for return statement */
    if (strncmp(l->src + l->pos, "return", 6) == 0 && (l->pos + 6 >= l->len || !isalnum((unsigned char)l->src[l->pos+6]))) {
        l->pos += 6;
        gml_ast *ret = ast_new(GML_AST_RETURN);
        if (ret) ret->left = parse_expression(l);
        skip_space_comments(l);
        if (l->pos < l->len && l->src[l->pos] == ';') l->pos++;
        return ret;
    }

    gml_ast *expr = parse_expression(l);
    if (expr) {
        skip_space_comments(l);
        if (l->pos < l->len && l->src[l->pos] == ';') l->pos++;
        gml_ast *stmt = ast_new(GML_AST_EXPR_STMT);
        if (stmt) stmt->left = expr;
        return stmt;
    }

    return NULL;
}

int gml_parse_program(const char *code, gml_ast **out_ast, char *errbuf, size_t errbuf_size) {
    if (!code || !out_ast) {
        if (errbuf && errbuf_size > 0) snprintf(errbuf, errbuf_size, "invalid argument");
        return 0;
    }
    gml_lexer l = { code, 0, strlen(code), {0} };
    gml_ast *block = ast_new(GML_AST_BLOCK);

    while (l.pos < l.len) {
        skip_space_comments(&l);
        if (l.pos >= l.len) break;
        if (l.src[l.pos] == ';') { l.pos++; continue; }
        gml_ast *stmt = parse_statement(&l);
        if (stmt) ast_add_child(block, stmt);
        else break;
    }

    *out_ast = block;
    return 1;
}
