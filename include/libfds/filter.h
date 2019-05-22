#ifndef FDS_FILTER_H
#define FDS_FILTER_H

#define GENERATE_ENUM(ENUM)		ENUM,
#define GENERATE_STRING(STRING)	#STRING,


#define FDS_FILTER_DEFINE_DATATYPE_ENUM(e) \
	e(FDS_FILTER_DATATYPE_STR) \
	e(FDS_FILTER_DATATYPE_UINT)

typedef enum fds_filter_datatype {
	FDS_FILTER_DEFINE_DATATYPE_ENUM(GENERATE_ENUM)
} fds_filter_datatype_t;

extern const char *fds_filter_datatype_str[];


typedef struct fds_filter_data {
	fds_filter_datatype_t type;
	union {
		const char *str;
		unsigned long uint;
	};
} fds_filter_data_t;


#define FDS_FILTER_DEFINE_OP_ENUM(e) \
	e(FDS_FILTER_OP_NONE) \
	e(FDS_FILTER_OP_ADD) \
	e(FDS_FILTER_OP_MUL) \
	e(FDS_FILTER_OP_SUB) \
	e(FDS_FILTER_OP_DIV) \
	e(FDS_FILTER_OP_MOD) \
	e(FDS_FILTER_OP_UMINUS) \
	e(FDS_FILTER_OP_BITNOT) \
	e(FDS_FILTER_OP_BITAND) \
	e(FDS_FILTER_OP_BITOR) \
	e(FDS_FILTER_OP_BITXOR) \
	e(FDS_FILTER_OP_NOT) \
	e(FDS_FILTER_OP_AND) \
	e(FDS_FILTER_OP_OR) \
	e(FDS_FILTER_OP_EQ) \
	e(FDS_FILTER_OP_NE) \
	e(FDS_FILTER_OP_GT) \
	e(FDS_FILTER_OP_LT) \
	e(FDS_FILTER_OP_GE) \
	e(FDS_FILTER_OP_LE) \
	e(FDS_FILTER_OP_CONST) \
	e(FDS_FILTER_OP_ID) \
	e(FDS_FILTER_OP_ENUM_COUNT)

typedef enum fds_filter_op { 
	FDS_FILTER_DEFINE_OP_ENUM(GENERATE_ENUM)
} fds_filter_op_t;

extern const char *fds_filter_op_str[];


typedef struct fds_filter fds_filter_t;

typedef int (*fds_data_func_t)(fds_filter_t *filter, int identifier_id, /*out*/ fds_filter_data_t *data);
typedef int (*fds_lookup_func_t)(fds_filter_t *filter, const char *identifier_name, /*out*/ int *identifier_id);
// or maybe?
#if 0
typedef int (*fds_data_func_t)(fds_filter_t *filter, /*out*/ fds_filter_data_t *data);
typedef int (*fds_lookup_func_t)(fds_filter_t *filter, const char *identifier_name, /*out*/ fds_data_func_t data_func);
#endif


typedef struct fds_filter_ast_node {
	fds_filter_op_t op;
	union {
		struct {
			struct fds_filter_ast_node *left;
			struct fds_filter_ast_node *right;
		};
		fds_filter_data_t *data;
	};
} fds_filter_ast_node_t;


typedef struct fds_filter {
	fds_filter_ast_node_t *ast;
	int error;
} fds_filter_t;


FDS_API int 
fds_filter_compile(const char *code, fds_filter_t **filter);

FDS_API int 
fds_filter_execute(fds_filter_t *filter);

FDS_API void 
fds_filter_destroy(fds_filter_t *filter);

FDS_API void 
fds_filter_ast_print_tree(fds_filter_ast_node_t *node);

#endif // FDS_FILTER_H






