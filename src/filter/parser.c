#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast_common.h"
#include "util.h"
#include "parser.h"
#include "scanner.h"
#include "error.h"

// specifies how to parse operators
struct operator_s {
    const char *symbol;
    int prec;             // operator precedence, higher binds first
    enum op_kind_e {
        OP_KIND_PREFIX,   // prefix unary operation, eg. -1
        OP_KIND_INFIX,    // infix binary operation, eg. 1 + 1
    } kind;
    enum op_assoc_e {
        OP_ASSOC_NONE,    // unary operators have no associativity
        OP_ASSOC_LEFT,
        OP_ASSOC_RIGHT,
    } assoc;
};

// the supported operators and how to parse them
struct operator_s op_parse_def_table[] = {
    { "+"       , 7, OP_KIND_PREFIX, OP_ASSOC_NONE },
    { "-"       , 7, OP_KIND_PREFIX, OP_ASSOC_NONE },

    { "*"       , 6, OP_KIND_INFIX , OP_ASSOC_LEFT },
    { "/"       , 6, OP_KIND_INFIX , OP_ASSOC_LEFT },
    { "%"       , 6, OP_KIND_INFIX , OP_ASSOC_LEFT },

    { "+"       , 5, OP_KIND_INFIX , OP_ASSOC_LEFT },
    { "-"       , 5, OP_KIND_INFIX , OP_ASSOC_LEFT },

    { "<<"      , 4, OP_KIND_INFIX , OP_ASSOC_LEFT },
    { ">>"      , 4, OP_KIND_INFIX , OP_ASSOC_LEFT },

    { "~"       , 3, OP_KIND_PREFIX, OP_ASSOC_NONE },
    { "&"       , 3, OP_KIND_INFIX , OP_ASSOC_LEFT },
    { "|"       , 3, OP_KIND_INFIX , OP_ASSOC_LEFT },
    { "^"       , 3, OP_KIND_INFIX , OP_ASSOC_LEFT },

    { "exists"  , 2, OP_KIND_PREFIX, OP_ASSOC_NONE },
    { "in"      , 2, OP_KIND_INFIX , OP_ASSOC_LEFT },
    { "contains", 2, OP_KIND_INFIX , OP_ASSOC_LEFT },

    { ""        , 1, OP_KIND_INFIX , OP_ASSOC_LEFT },
    { "=="      , 1, OP_KIND_INFIX , OP_ASSOC_LEFT },
    { "!="      , 1, OP_KIND_INFIX , OP_ASSOC_LEFT },
    { "<"       , 1, OP_KIND_INFIX , OP_ASSOC_LEFT },
    { ">"       , 1, OP_KIND_INFIX , OP_ASSOC_LEFT },
    { "<="      , 1, OP_KIND_INFIX , OP_ASSOC_LEFT },
    { ">="      , 1, OP_KIND_INFIX , OP_ASSOC_LEFT },

    { "not"     , 0, OP_KIND_PREFIX, OP_ASSOC_NONE },
    { "and"     , 0, OP_KIND_INFIX , OP_ASSOC_LEFT },
    { "or"      , 0, OP_KIND_INFIX , OP_ASSOC_LEFT },
};

// special keywords that can be prefixes of a const/field name
// eg. in addr, because normally names can't contain spaces
const char *name_prefixes[] = {
    "in", "out", "ingress", "egress", "src", "dst"
};


static bool
token_is_name_prefix(struct token_s token)
{
    for (int i = 0; i < CONST_ARR_SIZE(name_prefixes); i++) {
        if (token_is_symbol(token, name_prefixes[i])) {
            return true;
        }
    }
    return false;
}

// find an entry in the operator table matching the kind and symbol
struct operator_s *
find_operator(enum op_kind_e kind, const char *symbol)
{
    for (int i = 0; i < CONST_ARR_SIZE(op_parse_def_table); i++) {
        struct operator_s *o = &op_parse_def_table[i];
        if (o->kind == kind && strcmp(o->symbol, symbol) == 0) {
            return o;
        }
    }
    return NULL;
}

error_t parse_prefix_expr(struct scanner_s *scanner, fds_filter_ast_node_s **out_ast);

error_t
parse_infix_expr(struct scanner_s *scanner, int prec, fds_filter_ast_node_s **out_ast) {
    error_t err;

    fds_filter_ast_node_s *ast;
    err = parse_prefix_expr(scanner, &ast);
    if (err != NO_ERROR) {
        return err;
    }

    for (;;) {

        struct token_s token;
        err = next_token(scanner, &token);
        if (err != NO_ERROR) {
            destroy_ast(ast);

            return err;
        }

        struct operator_s *o = NULL;

        // check if the symbol is an infix operator and try to find its definition
        if (token.kind == TK_SYMBOL && (o = find_operator(OP_KIND_INFIX, token.symbol))) {
            // if the precedence of the next operator is lower (smaller binding power) then stop.
            // this returns control to the loop higher in the recursive call tree that is parsing lower precedence operators
            if (o->prec < prec) {
                break;
            }

            consume_token(scanner);

        } else {
            // if the token is any of the trailing tokens we have reached the right boundary of the expression
            if (token_is(token, TK_END) || token_is_symbol(token, ")") || token_is_symbol(token, ",") || token_is_symbol(token, "]")) {
                break;
            }

            // if we got here the next token is not a trailing symbol nor an infix operator,
            // try to match the "implicit operator", eg. for comparsion without == as in "ip 127.0.0.1 and port 80"
            o = find_operator(OP_KIND_INFIX, "");
            if (!o || o->prec < prec) {
                break;
            }
        }

        fds_filter_ast_node_s *right_expr;
        if (o->assoc == OP_ASSOC_LEFT) {
            // if the operation is left associative we want to only match operators with higher precedence
            // in the parsed subexpression that will be on the right side
            //
            // example: 1 + 2 + 3
            //            ^
            //            we are here
            //
            // we want the recursive parse call to only match the 2 and leave the next + to end up with a parse tree:
            //
            //                                     (+) <-- the call one level above in the recursive call tree is creating this node
            //                                    /    \
            //   we are creating this node ---> (+)    (4)
            //                                 /    \
            //    this is our left expr ---> (2)     (3) <--- this part is returned from the following call
            //
            err = parse_infix_expr(scanner, o->prec + 1, &right_expr);
        } else if (o->assoc == OP_ASSOC_RIGHT) {
            // if it's right associative we want to match operators with the same or higher precedence
            //
            // example: 2 ** 3 ** 4
            //            ^
            //            we are here
            //
            // as ** is right associative (** being the power operator i  n this example) we want to match
            // the following ** to end up with a parse tree:
            //
            //                                 (**) <--- we are creating this node
            //                                /    \
            //       this is our left --->  (2)     (**) <--- this part is returned from the following call
            //                                     /    \
            //                                   (3)     (4)
            err =  parse_infix_expr(scanner, o->prec, &right_expr);
        }
        if (err != NO_ERROR) {
            destroy_ast(ast);

            return err;
        }

        // create the resulting node from the left and right expression
        fds_filter_ast_node_s *new_ast = create_binary_ast_node(o->symbol, ast, right_expr);
        if (!new_ast) {
            destroy_ast(ast);
            destroy_ast(right_expr);
            return MEMORY_ERROR;
        }
        ast = new_ast;
    }

    *out_ast = ast;
    return NO_ERROR;
}

error_t
parse_prefix_expr(struct scanner_s *scanner, fds_filter_ast_node_s **out_ast)
{
    error_t err;

    struct token_s token;
    err = next_token(scanner, &token);
    if (err != NO_ERROR) {
        return err;
    }

    // parse expression in parentheses
    if (token_is_symbol(token, "(")) {
        const char *cursor_begin = token.cursor_begin;
        consume_token(scanner);

        // parse the inner expression
        fds_filter_ast_node_s *ast;
        err = parse_infix_expr(scanner, 0, &ast);
        if (err != NO_ERROR) {

            return err;
        }

        err = next_token(scanner, &token);
        if (err != NO_ERROR) {
            destroy_ast(ast);

            return err;
        }

        // check for the closing )
        if (!token_is_symbol(token, ")")) {
            destroy_ast(ast);
            return SYNTAX_ERROR(&token, "expected )");
        }

        ast->cursor_begin = cursor_begin;
        ast->cursor_end = token.cursor_end;
        consume_token(scanner);

        *out_ast = ast;
        return NO_ERROR;
    }

    // match name prefixes
    if (token_is_name_prefix(token)) {
        const char *cursor_begin = token.cursor_begin;

        // save the prefix and scan next token
        const char *prefix = token.symbol;
        consume_token(scanner);
        err = next_token(scanner, &token);
        if (err != NULL) {

            return err;
        }

        // remainder of the name has to follow
        if (!token_is(token, TK_NAME)) {
            return LEXICAL_ERROR(scanner, "expected name");
        }

        consume_token(scanner);

        fds_filter_ast_node_s *ast = create_ast_node("__name__");
        if (!ast) {
            return MEMORY_ERROR;
        }

        ast->name = malloc(strlen(token.name) + strlen(prefix) + 2);
        if (!ast->name) {
            destroy_ast(ast);
            return MEMORY_ERROR;
        }
        sprintf(ast->name, "%s %s", prefix, token.name);
        destroy_token(token);

        ast->cursor_begin = cursor_begin;
        ast->cursor_end = token.cursor_end;

        *out_ast = ast;
        return NO_ERROR;
    }

    // match list
    if (token_is_symbol(token, "[")) {
        const char *cursor_begin = token.cursor_begin;

        // consume the initial [
        consume_token(scanner);

        err = next_token(scanner, &token);
        if (err != NO_ERROR) {

            return err;
        }

        // create an empty list
        fds_filter_ast_node_s *list_node = create_ast_node("__list__");
        if (!list_node) {
            return MEMORY_ERROR;
        }
        // where to allocate the next list item node
        fds_filter_ast_node_s **item_node_ptr = &list_node->child;
        fds_filter_ast_node_s *parent = list_node;

        // loop until the closing ] is reached
        while (!token_is_symbol(token, "]")) {
            // match expression
            fds_filter_ast_node_s *expr_node;
            err = parse_infix_expr(scanner, 0, &expr_node);
            if (err != NO_ERROR) {
                destroy_ast(list_node);
                return err;
            }

            err = next_token(scanner, &token);
            if (err != NO_ERROR) {
                destroy_ast(list_node);
                destroy_ast(expr_node);
                return err;
            }

            // create the list item node
            fds_filter_ast_node_s *parent_node = *item_node_ptr;
            *item_node_ptr = create_unary_ast_node("__listitem__", expr_node);
            if (*item_node_ptr == NULL) {
                destroy_ast(list_node);
                destroy_ast(expr_node);
                return MEMORY_ERROR;
            }
            (*item_node_ptr)->parent = parent;
            parent = *item_node_ptr;
            // where to create the next list item node
            item_node_ptr = &(*item_node_ptr)->next;

            // if comma follows consume it, optional trailing comma is allowed in the list, eg. [1, 2, 3,] is okay
            if (token_is_symbol(token, ",")) {
                // TODO: free
                consume_token(scanner);
                err = next_token(scanner, &token);
                if (err != NO_ERROR) {
                    return err;
                }

            // if no comma then ] has to follow
            } else if (!token_is_symbol(token, "]")) {
                destroy_ast(list_node);
                return SYNTAX_ERROR(&token, "expected , or ]");
            }
        }
        consume_token(scanner);

        list_node->cursor_begin = cursor_begin;
        list_node->cursor_end = token.cursor_end;
        *out_ast = list_node;
        return NO_ERROR;
    }

    // match unary operator followed by expression
    if (token_is(token, TK_SYMBOL)) {
        const char *cursor_begin = token.cursor_begin;
        struct operator_s *o = find_operator(OP_KIND_PREFIX, token.symbol);
        if (o) {
            consume_token(scanner);

            fds_filter_ast_node_s *expr;
            err = parse_infix_expr(scanner, o->prec + 1, &expr);
            if (err != NO_ERROR) {
    
                return err;
            }

            fds_filter_ast_node_s *ast = create_unary_ast_node(o->symbol, expr);
            if (!ast) {
                destroy_ast(expr);
                return MEMORY_ERROR;
            }

            *out_ast = ast;
            return NO_ERROR;
        }
    }

    // match just name
    if (token_is(token, TK_NAME)) {
        consume_token(scanner);

        fds_filter_ast_node_s *ast = create_ast_node("__name__");
        if (!ast) {
            return MEMORY_ERROR;
        }
        ast->name = token.name;
        ast->cursor_begin = token.cursor_begin;
        ast->cursor_end = token.cursor_end;
        *out_ast = ast;
        return NO_ERROR;
    }

    // match value
    if (token_is(token, TK_LITERAL)) {
        consume_token(scanner);

        fds_filter_ast_node_s *ast = create_ast_node("__literal__");
        if (!ast) {
            return MEMORY_ERROR;
        }
        ast->value = token.literal.value;
        ast->datatype = token.literal.data_type;
        ast->flags |= FDS_FAF_CONST_SUBTREE;
        ast->flags |= FDS_FAF_DESTROY_VAL;
        ast->cursor_begin = token.cursor_begin;
        ast->cursor_end = token.cursor_end;
        *out_ast = ast;
        return NO_ERROR;
    }

    if (token_is(token, TK_END)) {
        return SYNTAX_ERROR(&token, "unexpected end of input");
    }

    return SYNTAX_ERROR(&token, "unexpected token");
}

error_t
parse_filter(struct scanner_s *scanner, fds_filter_ast_node_s **out_ast)
{
    error_t err;

    fds_filter_ast_node_s *ast;
    err = parse_infix_expr(scanner, 0, &ast);
    if (err != NO_ERROR) {
        return err;
    }

    struct token_s token;
    err = next_token(scanner, &token);
    if (err != NO_ERROR) {
        destroy_ast(ast);
        return err;
    }

    if (!token_is(token, TK_END)) {
        destroy_ast(ast);
        return LEXICAL_ERROR(scanner, "expected end of input");
    }

    fds_filter_ast_node_s *root_node = create_unary_ast_node("__root__", ast);
    if (!root_node) {
        destroy_ast(ast);
        return MEMORY_ERROR;
    }
    root_node->cursor_begin = ast->cursor_begin;
    root_node->cursor_end = ast->cursor_end;
    *out_ast = root_node;
    return NO_ERROR;
}
