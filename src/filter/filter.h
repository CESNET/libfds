#ifndef FDS_FILTER_FILTER_H
#define FDS_FILTER_FILTER_H

#include <libfds.h>

#ifndef NDEBUG
#define pdebug(fmt, ...) \
        do { fprintf(stderr, "%s:%d:%s(): " fmt "\n", __FILE__, __LINE__, __func__, __VA_ARGS__); } while (0)
#else
#define pdebug(fmt, ...) /* nothing */
#endif

struct error {
    const char *message;
    struct fds_filter_location location;
};

typedef void (*eval_func_t)(struct fds_filter *filter, struct eval_node *node);

struct eval_node {
    eval_func_t evaluate;
    struct eval_node *left;
    struct eval_node *right;
    unsigned char is_defined;
    unsigned char is_more;
    enum fds_filter_type type;
    enum fds_filter_type subtype;
    int identifier_id;
    union fds_filter_value value;
};

struct fds_filter {
    struct fds_filter_ast_node *ast;

    fds_filter_lookup_callback_t lookup_callback;
    fds_filter_data_callback_t data_callback;
    void *data; // The raw data
    void *context; // To be filled and operated on by the user
    unsigned char reset_context;

    struct eval_node *eval_tree;

    int error_count;
    struct error *errors;
};

void
error_message(struct fds_filter *filter, const char *format, ...);

void
error_location_message(struct fds_filter *filter, struct fds_filter_location location, const char *format, ...);

void
error_no_memory(struct fds_filter *filter);

void
error_destroy(struct fds_filter *filter);

const char *
type_to_str(int type);

const char *
ast_op_to_str(int op);

void
ast_print(FILE *outstream, struct fds_filter_ast_node *node);

struct fds_filter_ast_node *
ast_node_create();

void
ast_destroy(struct fds_filter_ast_node *node);

int
prepare_ast_nodes(struct fds_filter *filter, struct fds_filter_ast_node *node);

struct eval_node *
generate_eval_tree_from_ast(struct fds_filter *filter, struct fds_filter_ast_node *ast_node);

int
evaluate_eval_tree(struct fds_filter *filter, struct eval_node *eval_tree);

void
print_eval_tree(FILE *outstream, struct eval_node *node);

int
destroy_eval_tree(struct eval_node *eval_tree);

#endif // FDS_FILTER_FILTER_H
