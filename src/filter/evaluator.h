#ifndef FDS_FILTER_EVALUATOR_H
#define FDS_FILTER_EVALUATOR_H

#include <stdbool.h>
#include <libfds.h>

struct eval_node;

typedef void (*eval_func_t)(struct fds_filter *filter, struct eval_node *node);

struct eval_node {
    eval_func_t evaluate;
    struct eval_node *left;
    struct eval_node *right;
    bool is_defined;
    bool is_more;
    bool is_trie;
    bool is_alloc;
    enum fds_filter_data_type data_type;
    enum fds_filter_data_type data_subtype;
    int identifier_id;
    union fds_filter_value value;
};

struct eval_node *
generate_eval_tree(struct fds_filter *filter, struct fds_filter_ast_node *ast_node);

int
evaluate_eval_tree(struct fds_filter *filter, struct eval_node *eval_tree);

void
destroy_eval_tree(struct eval_node *eval_tree);

#endif // FDS_FILTER_EVALUATOR_H