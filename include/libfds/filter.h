#ifndef LIBFDS_FILTER_API_H
#define LIBFDS_FILTER_API_H

#ifdef __cplusplus
extern "C" {
#endif

#define FDS_FILTER_FAIL    -1

#define FDS_FILTER_OK       0
#define FDS_FILTER_OK_MORE  1

#define FDS_FILTER_YES      0
#define FDS_FILTER_NO       1

typedef struct fds_filter fds_filter_t;

/**
 * All the possible data types of a filter value
 *
 * FDS_FILTER_TYPE_NONE and FDS_FILTER_TYPE_COUNT are special values that are not valid filter data types
 */
enum fds_filter_data_type {
    FDS_FILTER_TYPE_NONE = 0,
    FDS_FILTER_TYPE_STR,
    FDS_FILTER_TYPE_UINT,
    FDS_FILTER_TYPE_INT,
    FDS_FILTER_TYPE_FLOAT,
    FDS_FILTER_TYPE_BOOL,
    FDS_FILTER_TYPE_IP_ADDRESS,
    FDS_FILTER_TYPE_MAC_ADDRESS,
    FDS_FILTER_TYPE_LIST,
    FDS_FILTER_TYPE_COUNT
};

/**
 * Union representing all the possible values a filter can use
 */
union fds_filter_value {
    struct {
        char *chars;
        uint64_t length;
    } string;
    struct {
        union fds_filter_value *items;
        uint64_t length;
    } list;
    uint8_t bool_;
    uint64_t uint_;
    int64_t int_;
    double float_;
    uint8_t bytes[18];
    struct {
		uint8_t version; // 4 or 6
        uint8_t mask;
        uint8_t bytes[16];
    } ip_address;
    uint8_t mac_address[6];
    void *pointer;
};

typedef union fds_filter_value fds_filter_value_t;

/**
 * Possible AST node operations
 */
enum fds_filter_ast_op {
    FDS_FILTER_AST_NONE = 0,
    FDS_FILTER_AST_ADD,
    FDS_FILTER_AST_MUL,
    FDS_FILTER_AST_SUB,
    FDS_FILTER_AST_DIV,
    FDS_FILTER_AST_MOD,
    FDS_FILTER_AST_UMINUS,
    FDS_FILTER_AST_BITNOT,
    FDS_FILTER_AST_BITAND,
    FDS_FILTER_AST_BITOR,
    FDS_FILTER_AST_BITXOR,
    FDS_FILTER_AST_NOT,
    FDS_FILTER_AST_AND,
    FDS_FILTER_AST_OR,
    FDS_FILTER_AST_EQ,
    FDS_FILTER_AST_NE,
    FDS_FILTER_AST_GT,
    FDS_FILTER_AST_LT,
    FDS_FILTER_AST_GE,
    FDS_FILTER_AST_LE,
    FDS_FILTER_AST_CONST,
    FDS_FILTER_AST_IDENTIFIER,
    FDS_FILTER_AST_LIST,
    FDS_FILTER_AST_LIST_ITEM,
    FDS_FILTER_AST_IN,
    FDS_FILTER_AST_CONTAINS,
    FDS_FILTER_AST_CAST,
    FDS_FILTER_AST_ANY,
    FDS_FILTER_AST_ROOT,
    FDS_FILTER_AST_COUNT
};

enum fds_filter_value_match_mode {
    FDS_FILTER_MATCH_MODE_NONE = 0,
    FDS_FILTER_MATCH_MODE_FULL,
    FDS_FILTER_MATCH_MODE_PARTIAL
};

enum fds_filter_identifier_type {
    FDS_FILTER_IDENTIFIER_FIELD,
    FDS_FILTER_IDENTIFIER_CONST
};

/**
 * The location of a node in the input text
 */
struct fds_filter_location {
    int first_line;
    int last_line;
    int first_column;
    int last_column;
};

/**
 * Node of an abstract syntax tree of the filter
 */
struct fds_filter_ast_node {
    enum fds_filter_ast_op op;

    struct fds_filter_ast_node *parent;

    struct fds_filter_ast_node *left;
    struct fds_filter_ast_node *right;

    const char *identifier_name;
    int identifier_id;
    enum fds_filter_identifier_type identifier_type;

    bool is_constant;
    bool is_trie;

    enum fds_filter_data_type type;
    enum fds_filter_data_type subtype; // In case of list
    enum fds_filter_value_match_mode match_mode;
    union fds_filter_value value;

    struct fds_filter_location location;
};

struct fds_filter_identifier_attributes {
    int id;
    enum fds_filter_identifier_type identifier_type;
    enum fds_filter_data_type type;
    enum fds_filter_data_type subtype;
    enum fds_filter_value_match_mode match_mode;
};

typedef int (*fds_filter_lookup_callback_t)
(const char *name, void *user_context, struct fds_filter_identifier_attributes *attributes);

typedef void (*fds_filter_const_callback_t)
(int id, void *user_context, union fds_filter_value *value);

typedef int (*fds_filter_field_callback_t)
(int id, void *user_context, int reset_flag, void *data, union fds_filter_value *value);

/**
 * Create an instance of a filter.
 *
 * @return The instance of the filter.
 */
FDS_API fds_filter_t *
fds_filter_create();

/**
 * Destroy an instance of a filter.
 *
 * @param   filter  The filter instance.
 */
FDS_API void
fds_filter_destroy(fds_filter_t *filter);

/**
 * Set the lookup callback of a filter.
 *
 * @param   filter          The filter instance.
 * @param   lookup_callback The lookup callback.
 */
FDS_API void
fds_filter_set_lookup_callback(fds_filter_t *filter, fds_filter_lookup_callback_t lookup_callback);

/**
 * Set the const callback of a filter.
 *
 * @param   filter          The filter instance.
 * @param   const_callback  The const callback.
 */
FDS_API void
fds_filter_set_const_callback(fds_filter_t *filter, fds_filter_const_callback_t const_callback);

/**
 * Set the field callback of a filter.
 *
 * @param   filter          The filter instance.
 * @param   field_callback  The field callback.
 */
FDS_API void
fds_filter_set_field_callback(fds_filter_t *filter, fds_filter_field_callback_t field_callback);

/**
 * Set the user context of a filter.
 *
 * The context is an arbitary value that is set by the user by this function, and is then passed as a parameter
 * to any subsequent calls of the data callback function. The purpose of this is to allow the user to store additional information
 * needed to correctly parse the data, for instance when parsing identifiers with multiple values to keep track of which values
 * have already been returned and which have not. This will most likely be a pointer to a struct that is to be defined, allocated
 * and then freed by the user.
 *
 * @param   filter       The filter instance.
 * @param   user_context The context.
 */
FDS_API void
fds_filter_set_user_context(fds_filter_t *filter, void *user_context);

/**
 * Get the user context of a filter.
 *
 * @param   filter  The filter instance.
 * @return The context as set by the user or NULL if no context was set.
 */
FDS_API void *
fds_filter_get_user_context(fds_filter_t *filter);

/**
 * Compile a filter from the filter expression.
 *
 * The lookup callback and context (if needed by the lookup function) has to be properly set before the call to this function.
 *
 * @param filter            The filter instance.
 * @param filter_expression The filter expression.
 * @return 1 on success, 0 if any errors occured.
 */
FDS_API int
fds_filter_compile(fds_filter_t *filter, const char *filter_expression);

/**
 * Evaluates the filter using the provided data.
 *
 * The filter has to be compiled and the data function has to be properly set before a call to this function.
 *
 * @param filter        The filter instance.
 * @param input_data    The input data.
 * @return FDS_API fds_filter_evaluate
 */
FDS_API int
fds_filter_evaluate(fds_filter_t *filter, void *input_data);

/**
 * Gets the abstract syntax tree of a compiled filter
 *
 * @param   filter   The filter instance.
 * @return  A pointer to the root of the abstract syntax tree.
 */
FDS_API const struct fds_filter_ast_node *
fds_filter_get_ast(fds_filter_t *filter);

/**
 * Gets the number of errors
 *
 * @param   filter  The filter instance.
 * @return  The error count.
 */
FDS_API int
fds_filter_get_error_count(fds_filter_t *filter);

/**
 * Gets the error message of the Nth error
 *
 * @param filter    The filter instance.
 * @param index     The index of the error.
 * @return  The error message if the error exists, else NULL.
 */
FDS_API const char *
fds_filter_get_error_message(fds_filter_t *filter, int index);

/**
 * Gets the location in the input text of the Nth error
 *
 * @param      filter       The filter instance.
 * @param      index        The index of the error.
 * @param[out] location     The location of the error in the input text.
 * @return  1 if the error exists and has a location set, else 0.
 */
FDS_API int
fds_filter_get_error_location(fds_filter_t *filter, int index, struct fds_filter_location *location);

/**
 * Print errors, return the number of errors printed.
 *
 * @param   filter      The filter instance.
 * @param   out_stream  The output stream to print the errors to.
 * @return  Number of errors printed.
 */
FDS_API int
fds_filter_print_errors(fds_filter_t *filter, FILE *out_stream);

#ifdef __cplusplus
}
#endif

#endif // LIBFDS_FILTER_API_H