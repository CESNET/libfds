#ifndef FDS_FILTER_INCLUDE_FILTER_H
#define FDS_FILTER_INCLUDE_FILTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libfds/api.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define FDS_FILTER_ERR_LEXICAL   -10
#define FDS_FILTER_ERR_SYNTAX    -11
#define FDS_FILTER_ERR_SEMANTIC  -12
#define FDS_FILTER_ERR_NOMEM     FDS_ERR_NOMEM
#define FDS_FILTER_ERROR         -1
#define FDS_FILTER_OK            FDS_OK
#define FDS_FILTER_OK_AND_MORE   1
#define FDS_FILTER_NOT_FOUND     2

#define FDS_FILTER_YES  0
#define FDS_FILTER_NO   1

#define FDS_FILTER_FLAG_CONST    1

struct fds_filter_error_s {
    int code;
    const char *message;
    const char *cursor_begin;
    const char *cursor_end;
};
typedef struct fds_filter_error_s fds_filter_error_t;


////////////////////////////////////////////////////////////////////////////////////////////////////
// Data types and values

typedef union fds_filter_value_u fds_filter_value_t;

struct fds_filter_ip_s {
    uint8_t version;
    uint8_t prefix;
    uint8_t addr[16];
};

struct fds_filter_str_s {
    uint64_t len;
    char *chars;
};

struct fds_filter_list_s {
    uint64_t len;
    fds_filter_value_t *items;
};

struct fds_filter_mac_s {
    uint8_t addr[6];
};

typedef struct fds_filter_ip_s fds_filter_ip_t;
typedef struct fds_filter_str_s fds_filter_str_t;
typedef struct fds_filter_list_s fds_filter_list_t;
typedef struct fds_filter_mac_s fds_filter_mac_t;
typedef int64_t fds_filter_int_t;
typedef uint64_t fds_filter_uint_t;
typedef double fds_filter_float_t;
typedef bool fds_filter_bool_t;

union fds_filter_value_u {
    fds_filter_ip_t ip;
    fds_filter_mac_t mac;
    fds_filter_list_t list;
    fds_filter_str_t str;
    fds_filter_int_t i;
    fds_filter_uint_t u;
    fds_filter_float_t f;
    fds_filter_bool_t b;
    void *p;
};

enum fds_filter_data_type_e {
    FDS_FILTER_DT_ANY = -1,

    FDS_FILTER_DT_NONE = 0,

    FDS_FILTER_DT_INT,
    FDS_FILTER_DT_UINT,
    FDS_FILTER_DT_FLOAT,
    FDS_FILTER_DT_STR,
    FDS_FILTER_DT_BOOL,
    FDS_FILTER_DT_IP,
    FDS_FILTER_DT_MAC,

    FDS_FILTER_DT_CUSTOM = 1 << 29,
    FDS_FILTER_DT_LIST = 1 << 30,
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct fds_filter_s {
    struct fds_filter_ast_node_s *ast;
    struct fds_filter_eval_s *eval;
    fds_filter_error_t *error;
};

typedef struct fds_filter_s fds_filter_t;
typedef struct fds_filter_opts_s fds_filter_opts_t;



/**
 * Look up identifier for its data type and properties during compilation
 *
 * // TODO: args
 *
 * @return  FDS_FILTER_OK or FDS_FILTER_ERROR
 */
typedef int fds_filter_lookup_callback_t(const char *name, int *out_id, int *out_datatype, int *out_flags);

/**
 * Get the value of an identifier if it's const during compilation
 *
 * // TODO: args
 *
 *
 */
typedef void fds_filter_const_callback_t(int id, fds_filter_value_t *out_value);

/**
 * Get the value of an identifier if it's field during evaluation
 *
 * Sets default value even if not found
 *
 * // TODO: args
 *
 * Returns one of FDS_FILTER_OK, FDS_FILTER_OK_AND_MORE, FDS_FILTER_NOT_FOUND
 */
typedef int fds_filter_field_callback_t(void *user_ctx, bool reset_ctx, int id, fds_filter_value_t *out_value);



////////////////////////////////////////////////////////////////////////////////////////////////////
// AST

enum fds_filter_ast_flags_e {
    FDS_FILTER_AST_FLAG_NONE = 0,
    FDS_FILTER_AST_FLAG_DESTROY_VAL = 0x1,
    FDS_FILTER_AST_FLAG_CONST_SUBTREE = 0x2,
    FDS_FILTER_AST_FLAG_MULTIPLE_EVAL_SUBTREE = 0x4,
};

struct fds_filter_ast_node_s {
    const char *symbol;
    union {
        struct {
            struct fds_filter_ast_node_s *left;
            struct fds_filter_ast_node_s *right;
        };
        struct fds_filter_ast_node_s *operand;
        struct {
            struct fds_filter_ast_node_s *item;
            struct fds_filter_ast_node_s *next;
        };
    };

    union fds_filter_value_u value;

    char *name;
    int id;

    int data_type;
    int flags;

    // the position of the node in the input text, for error message purposes
    const char *cursor_begin;
    const char *cursor_end;
};

////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////
// Operations

enum fds_filter_op_flags_e {
    FDS_FILTER_OP_FLAG_NONE = 0,
    FDS_FILTER_OP_FLAG_DESTROY = 0x1,
    FDS_FILTER_OP_FLAG_OVERRIDE = 0x2,
};

typedef void fds_filter_eval_func_t(fds_filter_value_t *left, fds_filter_value_t *right, fds_filter_value_t *result);

struct fds_filter_operation_s {
    const char *symbol;
    int out_data_type;
    int arg1_data_type;
    int arg2_data_type;
    fds_filter_eval_func_t *eval_func;
    int flags;
};

#define FDS_FILTER_DEFINE_INFIX_OP(LEFT_DT, SYMBOL, RIGHT_DT, FUNC, OUT_DT) \
    { .symbol = (SYMBOL), .arg1_data_type = (LEFT_DT), .arg2_data_type = (RIGHT_DT), .out_data_type = (OUT_DT), .eval_func = (FUNC) }
#define FDS_FILTER_DEFINE_PREFIX_OP(SYMBOL, OPERAND_DT, FUNC, OUT_DT) \
    { .symbol = (SYMBOL), .arg1_data_type = (OPERAND_DT), .arg2_data_type = FDS_FILTER_DT_NONE, .out_data_type = (OUT_DT), .eval_func = (FUNC) }
#define FDS_FILTER_DEFINE_CAST_OP(FROM_DT, FUNC, TO_DT) \
    { .symbol = "__cast__", .arg1_data_type = (FROM_DT), .arg2_data_type = FDS_FILTER_DT_NONE, .out_data_type = (TO_DT), .eval_func = (FUNC) }
#define FDS_FILTER_DEFINE_CONSTRUCTOR(DT, FUNC) \
    { .symbol = "__create__", .arg1_data_type = (DT), .arg2_data_type = FDS_FILTER_DT_NONE, .out_data_type = FDS_FILTER_DT_NONE, .eval_func = (FUNC) }
#define FDS_FILTER_DEFINE_DESTRUCTOR(DT, FUNC) \
    { .symbol = "__destroy__", .arg1_data_type = (DT), .arg2_data_type = FDS_FILTER_DT_NONE, .out_data_type = FDS_FILTER_DT_NONE, .eval_func = (FUNC) }

////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
// Options
FDS_API struct fds_filter_opts_s *
fds_filter_create_default_opts();

FDS_API void
fds_filter_opts_set_lookup_callback(fds_filter_opts_t *opts, fds_filter_lookup_callback_t *lookup_callback);

FDS_API void
fds_filter_opts_set_const_callback(fds_filter_opts_t *opts, fds_filter_const_callback_t *const_callback);

FDS_API void
fds_filter_opts_set_field_callback(fds_filter_opts_t *opts, fds_filter_field_callback_t *field_callback);

FDS_API int
fds_filter_opts_add_operation(fds_filter_opts_t *opts, struct fds_filter_operation_s operation);

FDS_API int
fds_filter_opts_extend_operations(fds_filter_opts_t *opts, struct fds_filter_operation_s *operations, size_t num_operations);

FDS_API void
fds_filter_destroy_opts(fds_filter_opts_t *opts);
////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////
// Main API

FDS_API int
fds_filter_compile(fds_filter_t **filter, const char *input_expr, fds_filter_opts_t *opts);

FDS_API int
fds_filter_evaluate(fds_filter_t *filter, void *data);

FDS_API void
fds_filter_destroy(fds_filter_t *filter);

FDS_API fds_filter_error_t *
fds_filter_get_error(fds_filter_t *filter);

FDS_API void
fds_filter_set_user_ctx(fds_filter_t *filter, void *user_ctx);

FDS_API void *
fds_filter_get_user_ctx(fds_filter_t *filter);

////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif


#endif