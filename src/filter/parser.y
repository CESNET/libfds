%code {
    #include <stdio.h>
    #include "parser.h"
    #include "scanner.h"
    extern int yylex();
    extern int yyparse();
    void yyerror();
}

%code requires {
    #include <libfds.h>
    #include "ast.h"
    #define YYDEBUG 1
    extern int yydebug;
}

%define api.pure full
%lex-param {void *scanner}
%parse-param {fds_filter_t *filter} {void *scanner} 

%union {
    char *s;
    long n;
    double d;
    fds_filter_ast_node_t *a;
}

%token AND "and" OR "or" NOT "not"
%token EQ "==" NE "!=" GT ">" LT "<" GE ">=" LE "<="
%token EXISTS "exists" CONTAINS "contains" IN "in"
%token OPAREN "(" CPAREN ")"
%token OBRACKET "[" CBRACKET "]"
%token PLUS "+" MINUS "-" UMINUS STAR "*" SLASH "/" PERCENT "%"
%token AMPERSAND "&" PIPE "|" CARET "^" TILDE "~" ASTERISK "!"
%token COMMA "," END "end"

%token <s> IDENTIFIER STRING
%token <n> NUMBER
%token <d> FLOATNUM
%type <a> expr term comparsion condition filter

%left "|" "&" "^"
%precedence "~"
%left "+" "-"
%left "*" "/" "%" 
%precedence UMINUS

%left "and" "or"
%precedence "not"

%%

filter: condition END { filter->ast = $1; }

condition: condition "and" condition  { printf("and"); $$ = ast_create(FDS_FILTER_OP_AND, $1, $3); }
         | condition "or" condition  { printf("or"); $$ = ast_create(FDS_FILTER_OP_OR, $1, $3); }
         | "not" condition { $$ = ast_create(FDS_FILTER_OP_NOT, $2, NULL); }
         | "(" condition ")" { $$ = $2; }
         | comparsion { $$ = $1; }

comparsion: expr "==" expr { $$ = ast_create(FDS_FILTER_OP_EQ, $1, $3); }
          | expr "!=" expr { $$ = ast_create(FDS_FILTER_OP_NE, $1, $3); }
          | expr "<" expr { $$ = ast_create(FDS_FILTER_OP_LT, $1, $3); }
          | expr ">" expr { $$ = ast_create(FDS_FILTER_OP_GT, $1, $3); }
          | expr "<=" expr { $$ = ast_create(FDS_FILTER_OP_LE, $1, $3); }
          | expr ">=" expr { $$ = ast_create(FDS_FILTER_OP_GE, $1, $3); }
          | expr expr { $$ = ast_create(FDS_FILTER_OP_EQ, $1, $2); }
          | expr { $$ = $1; }

expr: expr "|" expr { $$ = ast_create(FDS_FILTER_OP_BITOR, $1, $3); }
    | expr "&" expr { $$ = ast_create(FDS_FILTER_OP_BITAND, $1, $3); }
    | expr "^" expr { $$ = ast_create(FDS_FILTER_OP_BITXOR, $1, $3); }
    | "~" expr { $$ = ast_create(FDS_FILTER_OP_BITNOT, $2, NULL); }
    | expr "+" expr { $$ = ast_create(FDS_FILTER_OP_ADD, $1, $3); }
    | expr "-" expr { $$ = ast_create(FDS_FILTER_OP_SUB, $1, $3); }
    | expr "*" expr { $$ = ast_create(FDS_FILTER_OP_MUL, $1, $3); }
    | expr "/" expr { $$ = ast_create(FDS_FILTER_OP_DIV, $1, $3); }
    | expr "%" expr { $$ = ast_create(FDS_FILTER_OP_MOD, $1, $3); }
    | "-" %prec UMINUS expr { $$ = ast_create(FDS_FILTER_OP_UMINUS, $2, NULL); }
    | "(" expr ")" { $$ = $2; }
    | term { $$ = $1; }

term: NUMBER { $$ = ast_create(FDS_FILTER_OP_CONST, NULL, NULL); }
    | FLOATNUM { $$ = ast_create(FDS_FILTER_OP_CONST, NULL, NULL); }
    | STRING { $$ = ast_create(FDS_FILTER_OP_CONST, NULL, NULL); }
    | IDENTIFIER { printf("id"); $$ = ast_create(FDS_FILTER_OP_ID, NULL, NULL); }

%%

void yyerror(fds_filter_t *filter, void *scanner, char *err) {
    printf("*** Error: %s\n", err);
}