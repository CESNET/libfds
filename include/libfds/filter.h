#ifndef FDS_FILTER_INCLUDE_FILTER_H
#define FDS_FILTER_INCLUDE_FILTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libfds/api.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>  

typedef struct fds_filter_error {
    /** The error code                                                         */
    int code;
    /** The error message                                                      */
    const char *msg;

    /** Location of the error, both can be NULL depending on the type of error */
    /** Pointer to the beginning of the error location in the input expression */
    const char *cursor_begin;
    /** Pointer to the end of the error location in the input expression       */
    const char *cursor_end;
} fds_filter_error_s;


/**************************************************************************************************/
/* Data types and values of the filter                                                            */

/** Default data types and special bit values */
typedef enum fds_filter_datatype {
    FDS_FDT_ANY = -1, /**< Special value used in the filter, should not be used outside */

    FDS_FDT_NONE = 0,

    FDS_FDT_INT,
    FDS_FDT_UINT,
    FDS_FDT_FLOAT,
    FDS_FDT_STR,
    FDS_FDT_BOOL,
    FDS_FDT_IP,
    FDS_FDT_MAC,

    FDS_FDT_FLAGS,

    FDS_FDT_CUSTOM = 1 << 29, /**< Bit that indicates that the data type is custom      */ 
    FDS_FDT_LIST = 1 << 30,   /**< Bit that indicates that the data type is a list      */
} fds_filter_datatype_e;

typedef union fds_filter_value fds_filter_value_u; /**< Forward declaration because of list */

/** The IP address type */
typedef struct fds_filter_ip {
    /** The IP address version, 4 or 6   */ 
    uint8_t version;
    /** The prefix length of the address */
    uint8_t prefix;
    /** The address bytes                */
    uint8_t addr[16];
} fds_filter_ip_t;

/** The string type */
typedef struct fds_filter_str {
    /** The length of the string */
    uint64_t len;
    /** The characters, NOT NULL TERMINATED!!! */
    char *chars;
} fds_filter_str_t;

/** The list type */
typedef struct fds_filter_list {
    /** The number of items in the list */
    uint64_t len;
    /** The list values                 */
    fds_filter_value_u *items;
} fds_filter_list_t;

/** The mac address type (struct for the sake of consistency with ip address) */
typedef struct fds_filter_mac {
    uint8_t addr[6];
} fds_filter_mac_t;

/** The signed integer type */
typedef int64_t fds_filter_int_t;

/** The unsigned integer type */
typedef uint64_t fds_filter_uint_t;

/** The floating point type */
typedef double fds_filter_float_t;

/** The boolean type */
typedef bool fds_filter_bool_t;

typedef union fds_filter_value {
    fds_filter_ip_t ip;
    fds_filter_mac_t mac;
    fds_filter_list_t list;
    fds_filter_str_t str;
    fds_filter_int_t i;
    fds_filter_uint_t u;
    fds_filter_float_t f;
    fds_filter_bool_t b;
    void *p;
} fds_filter_value_u;

/**************************************************************************************************/

typedef struct fds_filter fds_filter_t;
typedef struct fds_filter_opts fds_filter_opts_t;


/** Indicates that the identifier is constant */
#define FDS_FILTER_FLAG_CONST    1

/**
 * Look up identifier for its data type and properties during compilation of the filter
 *
 * \param[in]  user_ctx      The user context set by fds_filter_set_user_ctx
 * \param[in]  name          The name of the identifier
 * \param[in]  other_name    The name of the first identifier on the other side if any 
 * \param[out] out_id        The id of the identifier that will be passed to the const/data callbacks.
 * \param[out] out_datatype  The data type of the identifier
 * \param[out] out_flags     The identifier flags. If set to FDS_FILTER_CONST_FLAG the identifier 
 *                           will be considered constant and the const callback will be used.
 *
 * \return  FDS_OK on success, 
 *          FDS_NOTFOUND if the identifier name is not recognized, which results in a filter error
 */
typedef int fds_filter_lookup_cb_t(
    void *user_ctx, const char *name, const char *other_name, int *out_id, int *out_datatype, int *out_flags);

/**
 * Get the value of a constant. 
 * An identifier is a constant if the FDS_FILTER_CONST_FLAG was set in the lookup callback. 
 *
 * \param[in]  user_ctx   The user context set by fds_filter_set_user_ctx
 * \param[in]  id         The id provided by the user in the lookup callback 
 * \param[out] out_value  The value of the constant 
 *
 */
typedef void fds_filter_const_cb_t(void *user_ctx, int id, fds_filter_value_u *out_value);

/**
 * Get the value of a field during evaluation
 *
 * Sets default value even if not found
 *
 * \param[in] user_ctx   The user context set by fds_filter_set_user_ctx
 * \param[in] reset_ctx  Indicates that a new field is being processed and the context should reset
 * // TODO: reset_ctx might not be the best name?    
 *
 * \return FDS_OK, FDS_OK_MORE or FDS_NOTFOUND
 *         FDS_OK       if the field is found and there are no more values following
 *         FDS_OK_MORE  if the field is found and more values may follow
 *         FDS_NOTFOUND if the field is not found, the callback is expected to set the default value
 * // TODO: might not be the best names either? 
 * //       maybe FDS_OK for more values following and FDS_FINAL or something for the last one is better? 
 */
typedef int fds_filter_data_cb_t(
    void *user_ctx, bool reset_ctx, int id, void *data, fds_filter_value_u *out_value);



////////////////////////////////////////////////////////////////////////////////////////////////////
// AST

enum fds_filter_fds_filter_ast_flags_e {
    FDS_FAF_NONE = 0,
    FDS_FAF_DESTROY_VAL = 0x1,
    FDS_FAF_CONST_SUBTREE = 0x2,
    FDS_FAF_MULTIPLE_EVAL_SUBTREE = 0x4,
};

typedef struct fds_filter_ast_node {
    const char *symbol;
    union {
        struct {
            struct fds_filter_ast_node *left;
            struct fds_filter_ast_node *right;
        };
        struct fds_filter_ast_node *child;
        struct {
            struct fds_filter_ast_node *item;
            struct fds_filter_ast_node *next;
        };
    };

    struct fds_filter_ast_node *parent;

    fds_filter_value_u value;

    char *name;
    int id;

    int datatype;
    int flags;

    // the position of the node in the input text, for error message purposes
    const char *cursor_begin;
    const char *cursor_end;
    
} fds_filter_ast_node_s;

////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////
// Operations

/**
 * Binary operation function type
 * 
 * \param[in]  arg1    The first (left) arg
 * \param[in]  arg2    The second (right) arg
 * \param[out] result  The output value
 */ 
typedef void fds_filter_binary_fn_t(fds_filter_value_u *arg1, fds_filter_value_u *arg2, fds_filter_value_u *result);

/**
 * Unary operation function type
 * 
 * \param[in]  arg1    The first (left) arg
 * \param[out] result  The output value
 */ 
typedef void fds_filter_unary_fn_t(fds_filter_value_u *arg, fds_filter_value_u *result);

/**
 * Cast function type
 * 
 * \param[in]  arg     The input value
 * \param[out] result  The output value
 */ 
typedef void fds_filter_cast_fn_t(fds_filter_value_u *arg, fds_filter_value_u *result);

/**
 * Constructor function type
 * 
 * \param[in]  arg     The input value
 * \param[out] result  The output value
 * 
 * \return FDS_OK on success, error code on failure
 */
typedef int fds_filter_constructor_fn_t(fds_filter_value_u *arg, fds_filter_value_u *result);

/**
 * Destructor function type
 * 
 * \param[in] arg  The value to destruct
 */
typedef void fds_filter_destructor_fn_t(fds_filter_value_u *arg);

typedef struct fds_filter_op {
    const char *symbol;
    int out_dt;
    int arg1_dt;
    int arg2_dt;
    union {
        fds_filter_unary_fn_t *unary_fn;
        fds_filter_binary_fn_t *binary_fn;
        fds_filter_cast_fn_t *cast_fn;
        fds_filter_constructor_fn_t *constructor_fn;
        fds_filter_destructor_fn_t *destructor_fn;
    };
} fds_filter_op_s;


#define FDS_FILTER_DEF_BINARY_OP(LEFT_DT, SYMBOL, RIGHT_DT, FUNC, OUT_DT) \
    {                                                                     \
      .symbol         = (SYMBOL),                                         \
      .arg1_dt        = (LEFT_DT),                                        \
      .arg2_dt        = (RIGHT_DT),                                       \
      .out_dt         = (OUT_DT),                                         \
      .binary_fn      = (FUNC)                                            \
    }                        
#define FDS_FILTER_DEF_UNARY_OP(SYMBOL, OPERAND_DT, FUNC, OUT_DT)         \
    {                                                                     \
      .symbol         = (SYMBOL),                                         \
      .arg1_dt        = (OPERAND_DT),                                     \
      .arg2_dt        = FDS_FDT_NONE,                               \
      .out_dt         = (OUT_DT),                                         \
      .unary_fn       = (FUNC)                                            \
    }                        
#define FDS_FILTER_DEF_CAST(FROM_DT, FUNC, TO_DT)                         \
    {                                                                     \
      .symbol         = "__cast__",                                       \
      .arg1_dt        = (FROM_DT),                                        \
      .arg2_dt        = FDS_FDT_NONE,                               \
      .out_dt         = (TO_DT),                                          \
      .cast_fn        = (FUNC)                                            \
    }
#define FDS_FILTER_DEF_CONSTRUCTOR(FROM_DT, FUNC, TO_DT)                  \
    {                                                                     \
      .symbol         = "__constructor__",                                \
      .arg1_dt        = (FROM_DT),                                        \
      .arg2_dt        = FDS_FDT_NONE,                               \
      .out_dt         = (TO_DT),                                          \
      .constructor_fn = (FUNC)                                            \
    }
#define FDS_FILTER_DEF_DESTRUCTOR(DT, FUNC)                               \
    {                                                                     \
      .symbol         = "__destructor__",                                 \
      .arg1_dt        = (DT),                                             \
      .arg2_dt        = FDS_FDT_NONE,                               \
      .out_dt         = FDS_FDT_NONE,                               \
      .destructor_fn  = (FUNC)                                            \
    }

#define FDS_FILTER_END_OP_LIST  { .symbol = NULL }

////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
// Options

/**
 * Creates the default options structure.
 * Because one options can be used by multiple filters, it has to be freed separately from the filter. 
 * 
 * \return the options
 */
FDS_API fds_filter_opts_t *
fds_filter_create_default_opts();

/**
 * Sets the lookup callback
 * 
 * \param[inout] opts  The options structure
 * \param[in]    cb    The callback
 */
FDS_API void
fds_filter_opts_set_lookup_cb(fds_filter_opts_t *opts, fds_filter_lookup_cb_t *cb);

/**
 * Sets the const callback
 * 
 * \param[inout] opts  The options structure
 * \param[in]    cb    The callback
 */
FDS_API void
fds_filter_opts_set_const_cb(fds_filter_opts_t *opts, fds_filter_const_cb_t *cb);

/**
 * Sets the data callback
 * 
 * \param[in] opts  The options structure
 * \param[in] cb    The callback
 */
FDS_API void
fds_filter_opts_set_data_cb(fds_filter_opts_t *opts, fds_filter_data_cb_t *cb);

/**
 * Adds a filter operation
 * 
 * \param[in] opts  The options structure
 * \param[in] op    The operation
 * 
 * \return pointer to the new op on success, NULL on failure
 */ 
FDS_API fds_filter_op_s *
fds_filter_opts_add_op(fds_filter_opts_t *opts, fds_filter_op_s op);

/**
 * Adds multiple filter operation
 * 
 * \param[in] opts    The options structure
 * \param[in] ops     The operations with a FDS_FILTER_END_OP_LIST marking the end
 * 
 * \return pointer to the first new op on success, NULL on failure
 */ 
FDS_API fds_filter_op_s *
fds_filter_opts_extend_ops(fds_filter_opts_t *opts, const fds_filter_op_s *ops);

/**
 * Destroys the options structure
 * 
 * \param[in] opts  The options structure
 */
FDS_API void
fds_filter_destroy_opts(fds_filter_opts_t *opts);

/**
 * Set the user context
 * 
 * \param[in] opts      The opts
 * \param[in] user_ctx  The user context
 */
FDS_API void
fds_filter_opts_set_user_ctx(fds_filter_opts_t *opts, void *user_ctx);

/**
 * Get the user context
 * 
 * \param[in] opts   The opts
 * \return the user context
 */
FDS_API void *
fds_filter_opts_get_user_ctx(const fds_filter_opts_t *opts);


////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////
// Main API

/**from an expression.
 * 
 * Creates the filter 
 * \param[out] filter   Pointer to where to allocate and construct the filter
 * \param[in]  expr         The filter expression
 * \param[in]  opts         The filter options
 * 
 * User context is an arbitary pointer provided by the user that's passed to all subsequent 
 * data callback calls as the user_ctx argument. It's intended to carry information about state 
 * that's required for the data callback function to correctly parse the data.
 */
FDS_API int
fds_filter_create(fds_filter_t **filter, const char *expr, const fds_filter_opts_t *opts);

/**
 * Evaluates the filter on the provided data.
 * 
 * \param[in] filter  The filter
 * \param[in] data    The data 
 *
 * \return true if the data passes the filter, false if not 
 */
FDS_API bool
fds_filter_eval(fds_filter_t *filter, void *data);

/**
 * Destroys the filter.
 * 
 * \param[in] filter  The filter to be destroyed.
 */
FDS_API void
fds_filter_destroy(fds_filter_t *filter);

/**
 * Gets the error from a filter.
 * 
 * If the filter parameter is NULL then memory error is assumed.
 * 
 * \param[in] filter  The filter
 * 
 * \return Pointer to the error, or NULL if there is no error.
 */
FDS_API fds_filter_error_s *
fds_filter_get_error(fds_filter_t *filter);

////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif


#endif
