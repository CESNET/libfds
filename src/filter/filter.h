#ifndef FDS_FILTER_FILTER_H
#define FDS_FILTER_FILTER_H

#include <stdbool.h>
#include <libfds.h>

#ifndef NDEBUG
#define pdebug(fmt, ...) \
        do { fprintf(stderr, "%s:%d:%s(): " fmt "\n", __FILE__, __LINE__, __func__,##__VA_ARGS__); } while (0)
#else
#define pdebug(fmt, ...) /* nothing */
#endif

#define CHECK_ERROR(x) \
    {  \
        int return_code = x; \
        if (return_code != FDS_FILTER_OK) { \
            return return_code; \
        } \
    }

struct error {
    char *message;
    struct fds_filter_location location;
};

struct eval_node;

typedef void (*eval_func_t)(struct fds_filter *filter, struct eval_node *node);

struct eval_node {
    eval_func_t evaluate;
    struct eval_node *left;
    struct eval_node *right;
    bool is_defined;
    bool is_more;
    bool is_trie;
    enum fds_filter_data_type type;
    enum fds_filter_data_type subtype;
    int identifier_id;
    union fds_filter_value value;
};

struct fds_filter {
    struct fds_filter_ast_node *ast;

    fds_filter_lookup_callback_t lookup_callback;
    fds_filter_const_callback_t const_callback;
    fds_filter_field_callback_t field_callback;
    void *data; // The raw data
    void *user_context; // To be filled and operated on by the user
    bool reset_context;

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
prepare_ast_nodes(struct fds_filter *filter, struct fds_filter_ast_node **node);

struct eval_node *
generate_eval_tree_from_ast(struct fds_filter *filter, struct fds_filter_ast_node *ast_node);

int
evaluate_eval_tree(struct fds_filter *filter, struct eval_node *eval_tree);

void
print_eval_tree(FILE *outstream, struct eval_node *node);

void
destroy_eval_tree(struct eval_node *eval_tree);


#endif // FDS_FILTER_FILTER_H
