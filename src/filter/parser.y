%code {

#include <stdio.h>
#include "parser.h"
#include "scanner.h"
#include "ast.h"

extern int yylex();
extern int yyparse();

}

%code requires {

#include <libfds.h>
#include "filter.h"
#define YYDEBUG 1
extern int yydebug;
#define YY_DECL int yylex(YYSTYPE *yylval_param, YYLTYPE *yylloc_param, struct fds_filter *filter, yyscan_t yyscanner)
typedef struct YYLTYPE YYLTYPE;
struct YYLTYPE {
    int first_line;
    int first_column;
    int last_line;
    int last_column;
};
#define YYLTYPE_IS_DECLARED 1
#define YYLTYPE_IS_TRIVIAL 1
void yyerror(YYLTYPE *loc, struct fds_filter *filter, void *scanner, char *err);

#define AST_NODE_(result, op_, left_, right_, location_) \
    struct fds_filter_ast_node *node; \
    if ((node = create_ast_node()) == NULL) { \
        no_memory_error(filter->error_list); \
        YYABORT; \
    } \
    node->node_type = op_; \
    node->left = left_; \
    node->right = right_; \
    if (left_ != NULL) { \
        ((struct fds_filter_ast_node *)left_)->parent = node; \
    } \
    if (right_ != NULL) { \
        ((struct fds_filter_ast_node *)right_)->parent = node; \
    } \
    node->location.first_line = location_.first_line; \
    node->location.last_line = location_.last_line; \
    node->location.first_column = location_.first_column ; \
    node->location.last_column = location_.last_column ; \
    result = node;

#define AST_CONST_(result, node_, location_) \
    struct fds_filter_ast_node *node; \
    if ((node = create_ast_node()) == NULL) { \
        no_memory_error(filter->error_list); \
        YYABORT; \
    } \
    node->node_type = FDS_FANT_CONST; \
    node->data_type = node_.data_type; \
    node->data_subtype = node_.data_subtype; \
    node->value = node_.value; \
    node->location.first_line = location_.first_line; \
    node->location.last_line = location_.last_line; \
    node->location.first_column = location_.first_column ; \
    node->location.last_column = location_.last_column ; \
    result = node;


#define AST_IDENTIFIER_(result, node_, location_) \
    struct fds_filter_ast_node *node; \
    if ((node = create_ast_node()) == NULL) { \
        no_memory_error(filter->error_list); \
        YYABORT; \
    } \
    node->node_type = FDS_FANT_IDENTIFIER; \
    node->identifier_name = node_.identifier_name; \
    node->location.first_line = location_.first_line; \
    node->location.last_line = location_.last_line; \
    node->location.first_column = location_.first_column ; \
    node->location.last_column = location_.last_column ; \
    result = node;
}


%locations
%define api.pure full
%param {struct fds_filter *filter} {void *scanner}

%union {
    struct fds_filter_ast_node ast;
    struct fds_filter_ast_node *ast_ptr;
}

%token AND "and" OR "or" NOT "not"
%token EQ "==" NE "!=" GT ">" LT "<" GE ">=" LE "<="
%token EXISTS "exists" CONTAINS "contains" INSIDE "inside"
%token OPAREN "(" CPAREN ")"
%token OBRACKET "[" CBRACKET "]"
%token PLUS "+" MINUS "-" UMINUS STAR "*" SLASH "/" PERCENT "%"
%token AMPERSAND "&" PIPE "|" CARET "^" TILDE "~" ASTERISK "!"
%token COMMA "," END "end"

%token <ast> IDENTIFIER STR UINT INT FLOAT BOOL IP_ADDRESS MAC_ADDRESS
%token ERROR_ABORT
%type <ast_ptr> expr term comparsion condition filter list_items list

%left "|" "&" "^"
%precedence "~"
%left "+" "-"
%left "*" "/" "%"
%precedence UMINUS

%left "and" "or"
%precedence "not"
/*
%destructor {
    if ($$->data_type == FDS_FILTER_DATATYPE_STR) {
        free($$.value.str);
    }
} <data>

%destructor {
    ast_free($$);
} <ast>
*/

%%

filter: condition { AST_NODE_(filter->ast, FDS_FANT_ROOT, $1, NULL, @$) }

condition: condition "and" condition  { AST_NODE_($$, FDS_FANT_AND, $1, $3, @$) }
         | condition "or" condition  { AST_NODE_($$, FDS_FANT_OR, $1, $3, @$) }
         | "not" condition { AST_NODE_($$, FDS_FANT_NOT, $2, NULL, @$) }
         | "(" condition ")" { $$ = $2; }
         | comparsion { $$ = $1; }
         | error { $$ = NULL; }

comparsion: expr "==" expr { AST_NODE_($$, FDS_FANT_EQ, $1, $3, @$) }
          | expr "!=" expr { AST_NODE_($$, FDS_FANT_NE, $1, $3, @$) }
          | expr "<" expr { AST_NODE_($$, FDS_FANT_LT, $1, $3, @$) }
          | expr ">" expr { AST_NODE_($$, FDS_FANT_GT, $1, $3, @$) }
          | expr "<=" expr { AST_NODE_($$, FDS_FANT_LE, $1, $3, @$) }
          | expr ">=" expr { AST_NODE_($$, FDS_FANT_GE, $1, $3, @$) }
          | expr "inside" expr { AST_NODE_($$, FDS_FANT_IN, $1, $3, @$) }
          | expr "contains" expr { AST_NODE_($$, FDS_FANT_CONTAINS, $1, $3, @$) }
          | expr expr { AST_NODE_($$, FDS_FANT_IMPLICIT, $1, $2, @$) }
          | expr { $$ = $1; }

expr: expr "|" expr { AST_NODE_($$, FDS_FANT_BITOR, $1, $3, @$) }
    | expr "&" expr { AST_NODE_($$, FDS_FANT_BITAND, $1, $3, @$) }
    | expr "^" expr { AST_NODE_($$, FDS_FANT_BITXOR, $1, $3, @$) }
    | "~" expr { AST_NODE_($$, FDS_FANT_BITNOT, $2, NULL, @$) }
    | expr "+" expr { AST_NODE_($$, FDS_FANT_ADD, $1, $3, @$) }
    | expr "-" expr { AST_NODE_($$, FDS_FANT_SUB, $1, $3, @$) }
    | expr "*" expr { AST_NODE_($$, FDS_FANT_MUL, $1, $3, @$) }
    | expr "/" expr { AST_NODE_($$, FDS_FANT_DIV, $1, $3, @$) }
    | expr "%" expr { AST_NODE_($$, FDS_FANT_MOD, $1, $3, @$) }
    | "-" %prec UMINUS expr { AST_NODE_($$, FDS_FANT_UMINUS, $2, NULL, @$) }
    | "(" expr ")" { $$ = $2; }
    | term { $$ = $1; }
    | list { $$ = $1; }

list_items: list_items "," expr { AST_NODE_($$, FDS_FANT_LIST_ITEM, $1, $3, @$) }
          | expr { AST_NODE_($$, FDS_FANT_LIST_ITEM, NULL, $1, @$) }

list: "[" "]" { AST_NODE_($$, FDS_FANT_LIST, NULL, NULL, @$) }
    | "[" list_items "]" { AST_NODE_($$, FDS_FANT_LIST, $2, NULL, @$) }

term: UINT { AST_CONST_($$, $1, @$) }
    | INT { AST_CONST_($$, $1, @$) }
    | FLOAT { AST_CONST_($$, $1, @$) }
    | STR { AST_CONST_($$, $1, @$) }
    | BOOL { AST_CONST_($$, $1, @$) }
    | IP_ADDRESS { AST_CONST_($$, $1, @$) }
    | MAC_ADDRESS { AST_CONST_($$, $1, @$) }
    | IDENTIFIER { AST_IDENTIFIER_($$, $1, @$) }

filter: ERROR_ABORT { YYABORT; }

%%

void yyerror(YYLTYPE *location, struct fds_filter *filter, void *scanner, char *error_message) {
    (void)scanner;
    struct fds_filter_location location_;
    location_.first_line = location->first_line;
    location_.last_line = location->last_line;
    location_.first_column = location->first_column;
    location_.last_column = location->last_column;
    add_error_location_message(filter->error_list, location_, "%s", error_message);
}
