/* Do not call yywrap on end of file */
%option noyywrap

/* Eliminates global variables, instead a pointer to yylex and other lex functions 
   is passed that has to be allocated with yylex_init -> int yylex ( yyscan_t scanner ); */
%option reentrant

/* Also passes lvalp to yylex -> int yylex ( YYSTYPE * lvalp, yyscan_t scanner ); */
%option bison-bridge

%{
    #include "parser.h"
%}

%%

(=|==|eq|EQ)               { return EQ; }
(!=|ne|NE)                 { return NE; }
(<|lt|LT|less|LESS)        { return LT; }
(>|gt|GT|greater|GREATER)  { return GT; }
(<=|le|LE)                 { return LE; }
(>=|ge|GE)                 { return GE; }
"("                        { return OPAREN; }
")"                        { return CPAREN; }
(&&|and|AND)               { return AND; }
(\|\||or|OR)               { return OR; }
(!|not|NOT)                { return NOT; }
(exists|EXISTS)            { return EXISTS; }
(in|IN)                    { return IN; }
(contains|CONTAINS)        { return CONTAINS; }
"+"                        { return PLUS; }
"-"                        { return MINUS; }
"*"                        { return STAR; }
"/"                        { return SLASH; }
"%"                        { return PERCENT; }
"&"                        { return AMPERSAND; }
"|"                        { return PIPE; }
"~"                        { return TILDE; }
"^"                        { return CARET; }
"["                        { return OBRACKET; }
"]"                        { return CBRACKET; }
","                        { return COMMA; }
"!"                        { return ASTERISK; }
\s                         { /* do nothing */ }
<<EOF>>                    { return END; }

((src|dst|in|out|ingress|egress)\s+)?[a-zA-Z][a-zA-Z0-9:\-\.]* { 
    yylval_param->s = strdup(yytext);
    /* TODO: handle error */
    return IDENTIFIER; 
}

\"(\\.|[^\"])*\" { 
    yylval_param->s = strdup(yytext);
    /* TODO: handle error */
    return STRING;  
}

[0-9]+ { 
    yylval_param->n = atoi(yytext); /* TODO: convert to long */
    return NUMBER;
}

%%