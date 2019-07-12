#include <libfds.h>
#include "filter.h"
#include "parser.h"
#include "scanner.h"

// TODO: clean this up, this is for debugging purposes for now
typedef void *yyscan_t;
extern int yydebug;

fds_filter_t *
fds_filter_create()
{
    fds_filter_t *filter = malloc(sizeof(fds_filter_t));
    if (filter == NULL) {
        return NULL;
    }
    filter->lookup_callback = NULL;
    filter->data_callback = NULL;
    filter->ast = NULL;
    filter->eval_tree = NULL;
    filter->reset_context = 0;
    filter->context = NULL;
    filter->data = NULL;
    filter->error_count = 0;
    filter->errors = NULL;
    return filter;
}

void
fds_filter_set_lookup_callback(fds_filter_t *filter, fds_filter_lookup_callback_t lookup_callback)
{
    assert(filter != NULL);
    filter->lookup_callback = lookup_callback;
}

void
fds_filter_set_data_callback(fds_filter_t *filter, fds_filter_data_callback_t data_callback)
{
    assert(filter != NULL);
    filter->data_callback = data_callback;
}

void
fds_filter_set_context(fds_filter_t *filter, void *context)
{
    assert(filter != NULL);
    filter->context = context;
}

void *
fds_filter_get_context(fds_filter_t *filter)
{
    assert(filter != NULL);
    return filter->context;
}

int
fds_filter_compile(fds_filter_t *filter, const char *filter_expression)
{
    assert(filter != NULL);
    assert(filter->lookup_callback != NULL);

    // TODO: clean this mess up
    yyscan_t scanner;
    yydebug = 0;
    yylex_init(&scanner);
    YY_BUFFER_STATE buffer = yy_scan_string(filter_expression, scanner);
    yyparse(filter, scanner);
    pdebug("Finished parsing", 0);
    yy_delete_buffer(buffer, scanner);
    yylex_destroy(scanner);

    if (fds_filter_get_error_count(filter) != 0) {
        pdebug("Parsing failed", 0);
        return 0;
    }
    pdebug("Filter compiled - input: %s", filter_expression);
    ast_print(stderr, filter->ast);

    if (!prepare_ast_nodes(filter, filter->ast)) {
        pdebug("Prepare nodes failed", 0);;
        return 0;
    }
    pdebug("After semantic", 0);
    ast_print(stderr, filter->ast);

    pdebug("After generate", 0);
    struct eval_node *eval_tree = generate_eval_tree_from_ast(filter, filter->ast);
    if (!eval_tree) {
        pdebug("Generate eval tree from ast failed", 0);
        return 0;
    }
    print_eval_tree(stderr, eval_tree);
    filter->eval_tree = eval_tree;

    return 1;
}

int
fds_filter_evaluate(fds_filter_t *filter, void *input_data)
{
    assert(filter != NULL);
    assert(filter->lookup_callback != NULL);
    assert(filter->data_callback != NULL);
    assert(filter->ast != NULL);
    assert(filter->eval_tree != NULL);

    filter->data = input_data;
    evaluate_eval_tree(filter, filter->eval_tree);
    // TODO: error handling
    return filter->eval_tree->value.uint_ != 0;
}

int
fds_filter_get_error_count(fds_filter_t *filter)
{
    return filter->error_count;
}

const char *
fds_filter_get_error_message(fds_filter_t *filter, int index)
{
    if (index >= filter->error_count) {
        return NULL;
    }
    return filter->errors[index].message;
}

int
fds_filter_get_error_location(fds_filter_t *filter, int index, struct fds_filter_location *location)
{
    if (index >= filter->error_count) {
        return 0;
    }
    if (filter->errors[index].location.first_line == -1) {
        return 0;
    }
    *location = filter->errors[index].location;
    return 1;
}

struct fds_filter_ast_node *
fds_filter_get_ast(fds_filter_t *filter)
{
    return filter->ast;
}

FDS_API int
fds_filter_print_errors(fds_filter_t *filter, FILE *out_stream)
{
    int i;
    pdebug("Error count: %d", fds_filter_get_error_count(filter));
    for (i = 0; i < fds_filter_get_error_count(filter); i++) {
        struct fds_filter_location location;
        const char *error_message = fds_filter_get_error_message(filter, i);
        fprintf(out_stream, "ERROR: %s", error_message);
        if (fds_filter_get_error_location(filter, i, &location)) {
            fprintf(out_stream, " on line %d:%d, column %d:%d",
                    location.first_line, location.last_line, location.first_column, location.last_column);
        }
        fprintf(out_stream, "\n");
    }
    return i;
}
