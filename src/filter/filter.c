#include <stdbool.h>
#include <libfds.h>
#include "filter.h"
#include "ast.h"
#include "error.h"
#include "debug.h"
#include "parser.h"
#include "scanner.h"

// TODO: clean this up, this is for debugging purposes for now
typedef void *yyscan_t;
extern int yydebug;

fds_filter_t *
fds_filter_create()
{
    fds_filter_t *filter = calloc(1, sizeof(fds_filter_t));
    if (filter == NULL) {
        return NULL;
    }
    filter->error_list = create_error_list();
    if (filter->error_list == NULL) {
        return NULL;
    }
    return filter;
}

void
fds_filter_destroy(fds_filter_t *filter)
{
    destroy_ast(filter->ast);
    destroy_eval_tree(filter->eval_tree);
    destroy_error_list(filter->error_list);
    free(filter);
}

void
fds_filter_set_lookup_callback(fds_filter_t *filter, fds_filter_lookup_callback_t lookup_callback)
{
    assert(filter != NULL);
    filter->lookup_callback = lookup_callback;
}

void
fds_filter_set_const_callback(fds_filter_t *filter, fds_filter_const_callback_t const_callback)
{
    assert(filter != NULL);
    filter->const_callback = const_callback;
}

void
fds_filter_set_field_callback(fds_filter_t *filter, fds_filter_field_callback_t field_callback)
{
    assert(filter != NULL);
    filter->field_callback = field_callback;
}

void
fds_filter_set_user_context(fds_filter_t *filter, void *user_context)
{
    assert(filter != NULL);
    filter->user_context = user_context;
}

void *
fds_filter_get_user_context(fds_filter_t *filter)
{
    assert(filter != NULL);
    return filter->user_context;
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
    yy_delete_buffer(buffer, scanner);
    yylex_destroy(scanner);

    if (fds_filter_get_error_count(filter) != 0) {
        PDEBUG("ERROR: Parsing failed!");
        return FDS_FILTER_FAIL;
    }
    PDEBUG("Filter compiled: \"%s\"", filter_expression);
    PDEBUG("====================== AST ======================");
    print_ast(stderr, filter->ast);
    PDEBUG("=================================================");

    int return_code;

    return_code = preprocess(filter);
    if (return_code != FDS_FILTER_OK) {
        PDEBUG("ERROR: Preprocess failed!");
        return return_code;
    }

    return_code = semantic_analysis(filter);
    if (return_code != FDS_FILTER_OK) {
        PDEBUG("ERROR: Semantic analysis failed!");
        return return_code;
    }

    return_code = optimize(filter);
    if (return_code != FDS_FILTER_OK) {
        PDEBUG("ERROR: Optimize failed!");
        return return_code;
    }

    PDEBUG("=================== Final AST ===================");
    print_ast(stderr, filter->ast);

    struct eval_node *eval_tree = generate_eval_tree(filter, filter->ast);
    if (eval_tree == NULL) {
        PDEBUG("ERROR: Generating eval tree failed!");
        return FDS_FILTER_FAIL;
    }
    PDEBUG("================ Final eval tree ================");
    print_eval_tree(stderr, eval_tree);
    filter->eval_tree = eval_tree;
    PDEBUG("=================================================");
    PDEBUG("");
    PDEBUG("");
    return FDS_FILTER_OK;
}

int
fds_filter_evaluate(fds_filter_t *filter, void *input_data)
{
    assert(filter != NULL);
    assert(filter->lookup_callback != NULL);
    assert(filter->const_callback != NULL);
    assert(filter->field_callback != NULL);
    assert(filter->ast != NULL);
    assert(filter->eval_tree != NULL);

    filter->data = input_data;

    int return_code = evaluate_eval_tree(filter, filter->eval_tree);
    if (return_code == FDS_FILTER_OK) {
        return filter->eval_tree->value.bool_ ? FDS_FILTER_YES : FDS_FILTER_NO;
    } else {
        return FDS_FILTER_FAIL;
    }
}

int
fds_filter_get_error_count(fds_filter_t *filter)
{
    return filter->error_list->count;
}

const char *
fds_filter_get_error_message(fds_filter_t *filter, int index)
{
    if (index >= filter->error_list->count) {
        return NULL;
    }
    return filter->error_list->errors[index].message;
}

int
fds_filter_get_error_location(fds_filter_t *filter, int index, struct fds_filter_location *location)
{
    if (index >= filter->error_list->count) {
        return 0;
    }
    struct fds_filter_location location_ = filter->error_list->errors[index].location;
    if (location_.first_line == 0) {
        return 0;
    }
    *location = location_;
    return 1;
}

const struct fds_filter_ast_node *
fds_filter_get_ast(fds_filter_t *filter)
{
    return filter->ast;
}

FDS_API int
fds_filter_print_errors(fds_filter_t *filter, FILE *output)
{
    int i;
    for (i = 0; i < fds_filter_get_error_count(filter); i++) {
        struct fds_filter_location location;
        const char *error_message = fds_filter_get_error_message(filter, i);
        fprintf(output, "ERROR: %s", error_message);
        if (fds_filter_get_error_location(filter, i, &location)) {
            fprintf(output, " on line %d:%d, column %d:%d",
                    location.first_line, location.last_line, location.first_column, location.last_column);
        }
        fprintf(output, "\n");
    }
    return i;
}

