/* Do not call yywrap on end of file */
%option noyywrap

/* Eliminates global variables, instead a pointer to yylex and other lex functions */
/* is passed that has to be allocated with yylex_init -> int yylex ( yyscan_t yyscanner ); */
%option reentrant

/* Also passes lvalp to yylex -> int yylex ( YYSTYPE * lvalp, yyscan_t yyscanner ); */
%option bison-bridge

%option bison-locations

%{
#include <arpa/inet.h>
#include <time.h>
#include "parser.h"

#define YY_USER_ACTION \
    yylloc->first_line = yylloc->last_line; \
    yylloc->first_column = yylloc->last_column; \
    for (int i = 0; yytext[i] != '\0'; i++) { \
        if (yytext[i] == '\n') { \
            yylloc->last_line++; \
            yylloc->last_column = 0; \
        } else { \
            yylloc->last_column++; \
        } \
    }
    
#define ERROR_(...) yyerror(yylloc, filter, yyscanner, __VA_ARGS__)

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
[ \t]                      { /* do nothing */ }

((src|dst|in|out|ingress|egress)\s+)?[a-zA-Z][a-zA-Z0-9@:\-\.]* { 
    yylval_param->ast.identifier_name = strdup(yytext);
    if (yylval_param->ast.identifier_name == NULL) {
        error_no_memory(filter);
        return ERROR_ABORT;
    }
    return IDENTIFIER; 
}

\"(\\.|[^\\"])*\" { 
    /* copy without surrounding quotes */
    char len = yyleng - 2; /* without the surrounding quotes */
    char *str = malloc(len + 1); /* + 1 for the terminating null byte */
    if (str == NULL) {
        error_no_memory(filter);
        return ERROR_ABORT;
    }
    str[len] = '\0';
    strncpy(str, &yytext[1], len); 
    for (int i = 0; i < len - 1; i++) {
        if (str[i] == '\\') { 
            memmove(&str[i], &str[i + 1], len - i);
            len -= 1;
        }
    }
    yylval_param->ast.type = FDS_FILTER_TYPE_STR;
    yylval_param->ast.value.string.length = len;
    yylval_param->ast.value.string.chars = str;
    return STR;
}
 
 /* int */
[0-9]+ { 
    char *unused;
    yylval_param->ast.type = FDS_FILTER_TYPE_INT;
    yylval_param->ast.value.int_ = strtoul(yytext, &unused, 10);
    return UINT;
}

 /* float */
[0-9]+\.[0-9]+ { 
    yylval_param->ast.type = FDS_FILTER_TYPE_FLOAT;
    yylval_param->ast.value.float_ = atof(yytext);
    return FLOAT;
}
    
 /* IPv4 address */
[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3} {
    yylval_param->ast.type = FDS_FILTER_TYPE_IP_ADDRESS;
	yylval_param->ast.value.ip_address.version = 4;
    if (inet_pton(AF_INET, yytext, yylval_param->ast.value.ip_address.bytes) != 1) {
        ERROR_("invalid IPv4 address");
        return 0;
    }
    return IP_ADDRESS;
}

 /* IPv6 address */
[0-9a-zA-Z]{0,2}(:[0-9a-zA-Z]{0,2}){15} {
    yylval_param->ast.type = FDS_FILTER_TYPE_IP_ADDRESS;
	yylval_param->ast.value.ip_address.version = 6;
    if (inet_pton(AF_INET6, yytext, yylval_param->ast.value.ip_address.bytes) != 1) {
        ERROR_("invalid IPv6 address");
        return 0; 
    }
    return IP_ADDRESS;
}

 /* MAC address */
[0-9a-zA-Z]{2}(:[0-9a-zA-Z]{2}){5} {
	yylval_param->ast.type = FDS_FILTER_TYPE_MAC_ADDRESS;
    sscanf(yytext, "%x:%x:%x:%x:%x:%x", 
           &yylval_param->ast.value.mac_address[0],
           &yylval_param->ast.value.mac_address[1],
           &yylval_param->ast.value.mac_address[2],
           &yylval_param->ast.value.mac_address[3],
           &yylval_param->ast.value.mac_address[4],
           &yylval_param->ast.value.mac_address[5]);
    return MAC_ADDRESS;
}

 /* ISO time */
[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}Z([-+][0-9]{2}([0-9]{2})?)? {
    struct tm tm;
    memset(&tm, 0, sizeof(struct tm));
    strptime(yytext, "%Y-%m-%dT%H:%M:%S%z", &tm);
    /* TODO: error handling? */
    unsigned long ns = mktime(&tm) * 1000000000;
    yylval_param->ast.type = FDS_FILTER_TYPE_UINT;
    yylval_param->ast.value.uint_  = ns;
}

 /* Time delta */
[0-9]+(\.[0-9]+)?(h|m|s|ms|us|ns)? {
    char suffix[3] = { '\0' };
    double value;
    sscanf(yytext, "%lf%2s", &value, &suffix);
    if      (strcmp(suffix, "h" ) == 0) { value *= 3600000000000; }
    else if (strcmp(suffix, "m" ) == 0) { value *= 60000000000;   }
    else if (strcmp(suffix, "s" ) == 0) { value *= 1000000000;    }
    else if (strcmp(suffix, "ms") == 0) { value *= 1000000;       }
    else if (strcmp(suffix, "us") == 0) { value *= 1000;          }
    else if (strcmp(suffix, "ns") == 0) { value *= 1;             }
    else                                { value *= 1000000000;    } /* seconds by default */
    yylval_param->ast.type = FDS_FILTER_TYPE_UINT; 
    yylval_param->ast.value.uint_ = value; 
    return TIMEDELTA;
}

. { return yytext[0]; }

%%

#undef ERROR_
