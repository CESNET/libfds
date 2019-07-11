#ifndef LIBFDS_FILTER_API_H
#define LIBFDS_FILTER_API_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct fds_filter fds_filter_t;

/**
 * @brief All the possible data types of a filter value
 *
 * FDS_FILTER_TYPE_NONE and FDS_FILTER_TYPE_COUNT are special values that are not valid filter data types
 */
enum fds_filter_type {
    FDS_FILTER_TYPE_NONE,
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
 * @brief Union representing all the possible values a filter can use
 */
union fds_filter_value {
    struct {
        int length;
        const char *chars; // TODO: come up with better names?
    } string;
    struct {
        int length;
        void *items; // TODO: come up with better names?
    } list;
    unsigned long uint_;
    long int_;
    double float_;
    unsigned char bytes[18];
    struct {
		unsigned char version; // 4 or 6
        unsigned char mask;
        unsigned char bytes[16];
    } ip_address;
    unsigned char mac_address[6];
};

typedef union fds_filter_value fds_filter_value_t;

/**
 * @brief Possible AST node operations
 */
enum fds_filter_ast_op {
    FDS_FILTER_AST_NONE,
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

/**
 * @brief The location of a node in the input text
 */
struct fds_filter_location {
    int first_line;
    int last_line;
    int first_column;
    int last_column;
};

/**
 * @brief Node of an abstract syntax tree of the filter
 */
struct fds_filter_ast_node {
    enum fds_filter_ast_op op;

    struct fds_filter_ast_node *left;
    struct fds_filter_ast_node *right;

    const char *identifier_name;
    int identifier_id;
    unsigned char identifier_is_constant;
    enum fds_filter_type type;
    enum fds_filter_type subtype; // In case of list
    union fds_filter_value value;

    struct fds_filter_location location;
};

/**
 * @brief The lookup callback function type
 *
 * TODO: explain more in detail
 *
 * @param       name         The name of the identifier that is being looked up
 * @param       data_context The data context provided by the user
 * @param[out]  id           The id of the identifier that will later be passed to data function calls
 * @param[out]  type         The data type of the identifier
 * @param[out]  is_constant  Whether the identifier value changes or not
 * @param[out]  value        The value of the identifier - only to be set if the identifier is constant!
 * @return TODO
 */
typedef int (*fds_filter_lookup_func_t)(const char *name, void *data_context, int *id, enum fds_filter_type *type, int *is_constant, union fds_filter_value *value);

/**
 * A structure to pass the lookup function arguments
 */
struct fds_filter_lookup_args {
    const char *name;                     /**< [in] The name of the identifier that is being looked up. */
    void *context;                        /**< [in] The context set by the user. */
    int *id;                              /**< [out] The id of the identifier that will be passed to subsequent calls to the data function. */
    enum fds_filter_type *type;           /**< [out] The data type of the identifier. */
    enum fds_filter_type *subtype;        /**< [out] In case type is a list, this is the data type of the elements of the list. */
    unsigned char *is_constant;           /**< [out] Whether the identifier has a constant value. If output value is set this has to be set to 1. */
    union fds_filter_value *output_value; /**< [out] The value of the identifier if the identifier is a constant. */
};

/**
 * The lookup callback function type
 *
 * TODO: explain more in detail
 *
 * @param[in,out]   args    The lookup function arguments.
 * @return 1 on success, 0 on failure
 */
typedef int (*fds_filter_lookup_callback_t)(struct fds_filter_lookup_args args);

/**
 * A structure to pass the data function arguments
 */
struct fds_filter_data_args {
    int id;                               /**< [in] The id of the identifier */
    void *context;                        /**< [in] The context set by the user */
    unsigned char reset;                  /**< [in] Indicates whether the context should reset */
    void *input_data;                     /**< [in] The input data to parse */
    union fds_filter_value *output_value; /**< [out] The parsed output data */
    unsigned char *has_more;              /**< [out] Whether the identifier has more values */
};

/**
 * The data (getter) callback function type
 *
 * TODO: explain more in detail
 *
 * @param[in,out]   args    The data function arguments.
 * @return  1 if the data was found, else 0
 */
typedef int (*fds_filter_data_callback_t)(struct fds_filter_data_args args);

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
 * Set the data callback of a filter.
 *
 * @param   filter          The filter instance.
 * @param   data_callback   The data callback.
 */
FDS_API void
fds_filter_set_data_callback(fds_filter_t *filter, fds_filter_data_callback_t data_callback);

/**
 * Set the context of a filter.
 *
 * The context is an arbitary value that is set by the user by this function, and is then passed as a parameter
 * to any subsequent calls of the data callback function. The purpose of this is to allow the user to store additional information
 * needed to correctly parse the data, for instance when parsing identifiers with multiple values to keep track of which values
 * have already been returned and which have not. This will most likely be a pointer to a struct that is to be defined, allocated
 * and then freed by the user.
 *
 * @param   filter  The filter instance.
 * @param   context The context.
 */
FDS_API void
fds_filter_set_context(fds_filter_t *filter, void *context);

/**
 * Get the context of a filter.
 *
 * @param   filter  The filter instance.
 * @return The context as set by the user or NULL if no context was set.
 */
FDS_API void *
fds_filter_get_context(fds_filter_t *filter);

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
 * @brief Gets the abstract syntax tree of a compiled filter
 *
 * @param   filter   The filter instance.
 * @return  A pointer to the root of the abstract syntax tree.
 */
FDS_API struct fds_filter_ast_node *
fds_filter_get_ast(fds_filter_t *filter);

/**
 * @brief Gets the number of errors
 *
 * @param   filter  The filter instance.
 * @return  The error count.
 */
FDS_API int
fds_filter_get_error_count(struct fds_filter *filter);

/**
 * @brief Gets the error message of the Nth error
 *
 * @param filter    The filter instance.
 * @param index     The index of the error.
 * @return  The error message if the error exists, else NULL.
 */
FDS_API const char *
fds_filter_get_error_message(struct fds_filter *filter, int index);

/**
 * @brief Gets the location in the input text of the Nth error
 *
 * @param      filter       The filter instance.
 * @param      index        The index of the error.
 * @param[out] location     The location of the error in the input text.
 * @return  1 if the error exists and has a location set, else 0.
 */
FDS_API int
fds_filter_get_error_location(struct fds_filter *filter, int index, struct fds_filter_location *location);

#ifdef __cplusplus
}
#endif

#endif // LIBFDS_FILTER_API_H