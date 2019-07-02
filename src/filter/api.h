#ifndef LIBFDS_FILTER_API_H
#define LIBFDS_FILTER_API_H

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
    FDS_FILTER_AST_IN,
    FDS_FILTER_AST_CONTAINS,
    FDS_FILTER_AST_CAST,
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

    struct fds_filter_ast_node_s *left;
    struct fds_filter_ast_node_s *right;

    const char *identifier_name;
    int identifier_id;
    enum fds_filter_type type;
    enum fds_filter_type subtype;
    union fds_filter_value value;

    struct fds_filter_location location;
};

/**
 * @brief The lookup callback function type
 * 
 * TODO: explain more in detail
 * 
 * @param      identifier_name  The name of the identifier that is being looked up
 * @param[out] identifier_id    The id of the identifier that will later be passed to data function calls
 * @param[out] is_constant      Whether the identifier value changes or not
 * @param[out] value            The value of the identifier - only to be set if the identifier is constant!
 * @return TODO
 */
typedef int (*fds_filter_lookup_func_t)(const char *identifier_name, int *identifier_id, int *is_constant, union fds_filter_value *value);

/**
 * @brief The data (getter) callback function type
 * 
 * TODO: explain more in detail
 * 
 * @param      identifier_id    The identifier id provided by the user in the lookup funciton
 * @param      data_context     The data context provided by the user 
 * @param      reset_context    Whether the context should reset
 * @param      data             The input data to parse
 * @param[out] value            The value parsed from the data
 * @return  TODO
 */
typedef int (*fds_filter_data_func_t)(int identifier_id, void *data_context, int reset_context, void *data, union fds_filter_value *value);

/**
 * @brief Creates an instance of filter from an input expression that can then be evaluated 
 * 
 * If 0 is returned *filter is set to NULL, there was not enough memory to even allocate the filter structure,
 * else you can check what went wrong using the error functions. 
 * 
 * @param      input        The input expression
 * @param      lookup_func  The lookup callback function 
 * @param      data_func    The data callback function
 * @param[out] filter       The compiled filter
 * @return 1 if the filter compiled successfully, 0 if any errors happened 
 */
FDS_API int 
fds_filter_create(const char *input, fds_filter_lookup_func_t lookup_func, fds_filter_data_func_t data_func, fds_filter_t **filter);

/**
 * @brief Evaluates the filter using the provided data
 * 
 * @param filter    The filter created by fds_filter_compile
 * @param data      The input data to run the filter over
 * @return 1 if the filter passes, else 0
 */
FDS_API int 
fds_filter_evaluate(fds_filter_t *filter, void *data);

/**
 * @brief Sets the data context that is then passed to the data callback function
 * 
 * The "data context" is an arbitary value that is set by the user by this function, and is then passed as a parameter
 * to any subsequent calls of the data callback function. The purpose of this is to allow the user to store additional information
 * needed to correctly parse the data, for instance when parsing identifiers with multiple values to keep track of which values
 * have already been returned and which have not. This will most likely be a pointer to a struct that is to be defined, allocated
 * and then freed by the user.
 * 
 * @param   filter   The filter
 * @param   context  The arbitary value provided by the user   
 */
FDS_API void
fds_filter_set_data_context(fds_filter_t *filter, void *context);

/**
 * @brief Returns the data context value that was set by a call to fds_filter_set_data_context
 * 
 * @param   filter  The filter 
 * @return  The data context as set by the user 
 */
FDS_API void *
fds_filter_get_data_context(fds_filter_t *filter);

/**
 * @brief Gets the abstract syntax tree of a compiled filter 
 * 
 * @param   filter   The filter
 * @return  A pointer to the root of the abstract syntax tree   
 */
FDS_API fds_filter_ast_node *
fds_filter_get_ast(fds_filter_t *filter);

/**
 * @brief Gets the number of errors
 * 
 * @param   filter  The filter 
 * @return  The error count
 */
FDS_API int 
fds_filter_get_error_count(struct fds_filter *filter);

/**
 * @brief Gets the error message of the Nth error
 * 
 * @param filter    The filter
 * @param index     The index of the error 
 * @return The error message if the error exists, else NULL 
 */
FDS_API const char *
fds_filter_get_error_message(struct fds_filter *filter, int index);

/**
 * @brief Gets the location in the input text of the Nth error
 * 
 * @param      filter       The filter
 * @param      index        The index of the error
 * @param[out] location   The location of the error in the input text
 * @return 1 if the error exists and has a location set, else 0
 */
FDS_API int
fds_filter_get_error_location(struct fds_filter *filter, int index, struct fds_filter_location *location);

#endif // LIBFDS_FILTER_API_H