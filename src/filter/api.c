#include "api.h"
#include "filter.h"
#include "parser.h"
#include "scanner.h"

// TODO: clean this up, this is for debugging purposes for now 
typedef void *yyscan_t;
extern int yydebug;

int 
fds_filter_create(const char *input, fds_filter_lookup_func_t lookup_func, fds_filter_data_func_t data_func, fds_filter_t **filter);
{
    *filter = malloc(sizeof(*filter));
    if (*filter == NULL) {
        pdebug("Cannot create filter - out of memory when allocating filter instance", 0);
        return 0;
    }

    (*filter)->ast = NULL;
    (*filter)->lookup_func = lookup_func;
    (*filter)->data_func = data_func;
	(*filter)->error_count = 0;
    (*filter)->errors = NULL;

    // TODO: clean this mess up
    yyscan_t scanner; 
    yydebug = 1;
    yylex_init(&scanner);
    YY_BUFFER_STATE buffer = yy_scan_string(input, scanner);
    yyparse(*filter, scanner);
    pdebug("Finished parsing", 0);
    yy_delete_buffer(buffer, scanner);
    yylex_destroy(scanner);

    if (fds_filter_get_error_count(*filter) != 0) {
        pdebug("Parsing failed", 0);
        return 0;
    }

    prepare_evaluation(*filter);

    pdebug("Filter compiled - input: %s", input);
    ast_print(stderr, (*filter)->ast);
    return 1;
}

void 
fds_filter_destroy(struct fds_filter_s *filter) 
{
    if (filter == NULL) {
        return;
    }
    ast_destroy(filter->ast);
    error_destroy(filter);
    free(filter);
}

int 
fds_filter_get_error_count(struct fds_filter *filter)
{
    return filter->error_count;
}

const char *
fds_filter_get_error_message(struct fds_filter *filter, int index)
{
    if (index < filter->error_count) {
        return NULL;
    }
    return filter->errors[index].message;
}

int
fds_filter_get_error_location(struct fds_filter *filter, int index, struct fds_filter_location *location)
{
    if (index < filter->error_count) {
        return 0;
    }
    if (filter->errors[index].location.first_line == -1) {
        return 0;
    }
    *location = filter->errors[index].location;
    return 1;
}

void
fds_filter_set_data_context(fds_filter_t *filter, void *context)
{
    filter->data_context = context;
}

void *
fds_filter_get_data_context(fds_filter_t *filter)
{
    return filter->data_context;
}

fds_filter_ast_node *
fds_filter_get_ast(fds_filter_t *filter)
{
    return filter->ast;
}