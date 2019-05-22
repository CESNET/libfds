#include <stdlib.h>

#include <libfds.h>
#include "parser.h"
#include "scanner.h"
#include "ast.h"

const char *fds_filter_op_str[] = {
    FDS_FILTER_DEFINE_OP_ENUM(GENERATE_STRING)
};

const char *fds_filter_datatype_str[] = {
    FDS_FILTER_DEFINE_DATATYPE_ENUM(GENERATE_STRING)
};


typedef void *yyscan_t;
extern int yydebug;

int 
fds_filter_compile(const char *code, fds_filter_t **filter)
{
    *filter = malloc(sizeof(fds_filter_t));
    if (*filter == NULL) {
        return FDS_ERR_NOMEM;
    }

    (*filter)->ast = NULL;
    (*filter)->error = 0;

    yyscan_t scanner; 
    yydebug = 1;
    yylex_init(&scanner);

    YY_BUFFER_STATE buffer = yy_scan_string(code, scanner);
    yyparse(*filter, scanner);
    yy_delete_buffer(buffer, scanner);

    yylex_destroy(scanner);

    return FDS_OK;
}

void 
fds_filter_destroy(fds_filter_t *filter) 
{
    ast_destroy(filter->ast);
    free(filter);
}

int 
fds_filter_execute(fds_filter_t *filter) 
{
    // TODO
    return 1;
}
