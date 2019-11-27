#ifndef FDS_FILTER_OPERATION_H
#define FDS_FILTER_OPERATION_H

#include <stdbool.h>

#include "array.h"
#include "values.h"

typedef struct fds_filter_operation_s operation_s;

#define OP_FLAG_NONE FDS_FILTER_OP_FLAG_NONE
#define OP_FLAG_DESTROY FDS_FILTER_OP_FLAG_DESTROY
#define OP_FLAG_OVERRIDE FDS_FILTER_OP_FLAG_OVERRIDE

#define DEFINE_INFIX_OP(LEFT_DT, SYMBOL, RIGHT_DT, FUNC, OUT_DT) \
    { .symbol = (SYMBOL), .arg1_data_type = (LEFT_DT), .arg2_data_type = (RIGHT_DT), .out_data_type = (OUT_DT), .eval_func = (FUNC) }
#define DEFINE_INFIX_OP_FLAGS(LEFT_DT, SYMBOL, RIGHT_DT, FUNC, OUT_DT, FLAGS) \
    { .symbol = (SYMBOL), .arg1_data_type = (LEFT_DT), .arg2_data_type = (RIGHT_DT), .out_data_type = (OUT_DT), .eval_func = (FUNC), .flags = (FLAGS)}
#define DEFINE_PREFIX_OP(SYMBOL, OPERAND_DT, FUNC, OUT_DT) \
    { .symbol = (SYMBOL), .arg1_data_type = (OPERAND_DT), .arg2_data_type = DT_NONE, .out_data_type = (OUT_DT), .eval_func = (FUNC) }
#define DEFINE_PREFIX_OP_FLAGS(SYMBOL, OPERAND_DT, FUNC, OUT_DT, FLAGS) \
    { .symbol = (SYMBOL), .arg1_data_type = (OPERAND_DT), .arg2_data_type = DT_NONE, .out_data_type = (OUT_DT), .eval_func = (FUNC), .flags = (FLAGS)}
#define DEFINE_CAST_OP(FROM_DT, FUNC, TO_DT) \
    { .symbol = "__cast__", .arg1_data_type = (FROM_DT), .arg2_data_type = DT_NONE, .out_data_type = (TO_DT), .eval_func = (FUNC) }
#define DEFINE_CONSTRUCTOR(DT, FUNC) \
    { .symbol = "__create__", .arg1_data_type = (DT), .arg2_data_type = DT_NONE, .out_data_type = DT_NONE, .eval_func = (FUNC) }
#define DEFINE_DESTRUCTOR(DT, FUNC) \
    { .symbol = "__destroy__", .arg1_data_type = (DT), .arg2_data_type = DT_NONE, .out_data_type = DT_NONE, .eval_func = (FUNC) }

static inline operation_s *
find_next_operation(struct array_s *operations, operation_s *prev, const char *symbol, int out_dt, int arg1_dt, int arg2_dt)
{
    ARRAY_FOR_CONTINUE(operations, prev, operation_s *, o) {
        if (strcmp(o->symbol, symbol) != 0) { continue; }
        if (arg1_dt != DT_ANY && arg1_dt != o->arg1_data_type) { continue; }
        if (arg2_dt != DT_ANY && arg2_dt != o->arg2_data_type) { continue; }
        if (out_dt  != DT_ANY && out_dt  != o->out_data_type ) { continue; }
        return o;
    }
    return NULL;
}

static inline operation_s *
find_operation(struct array_s *operations, const char *symbol, int out_dt, int arg1_dt, int arg2_dt)
{
    return find_next_operation(operations, NULL, symbol, out_dt, arg1_dt, arg2_dt);
}

static inline operation_s *
find_next_binary_operation(struct array_s *operations, operation_s *prev, const char *symbol, int arg1_dt, int arg2_dt)
{
    return find_next_operation(operations, prev, symbol, DT_ANY, arg1_dt, arg2_dt);
}

static inline operation_s *
find_binary_operation(struct array_s *operations, const char *symbol, int arg1_dt, int arg2_dt)
{
    return find_next_operation(operations, NULL, symbol, DT_ANY, arg1_dt, arg2_dt);
}

static inline operation_s *
find_next_unary_operation(struct array_s *operations, operation_s *prev, const char *symbol, int arg1_dt)
{
    return find_next_operation(operations, prev, symbol, DT_ANY, arg1_dt, DT_NONE);
}

static inline operation_s *
find_unary_operation(struct array_s *operations, const char *symbol, int arg1_dt)
{
    return find_next_operation(operations, NULL, symbol, DT_ANY, arg1_dt, DT_NONE);
}

static inline operation_s *
find_next_cast(struct array_s *operations, operation_s *prev, int from_datatype)
{
    return find_next_operation(operations, prev, "__cast__", DT_ANY, from_datatype, DT_NONE);
}

static inline operation_s *
find_cast(struct array_s *operations, int from_datatype, int to_datatype)
{
    return find_next_operation(operations, NULL, "__cast__", to_datatype, from_datatype, DT_NONE);
}

static inline bool
can_cast(struct array_s *operations, int from_datatype, int to_datatype)
{
    return from_datatype == to_datatype || find_cast(operations, from_datatype, to_datatype) != NULL;
}


static inline operation_s *
find_constructor(struct array_s *operations, int datatype)
{
    return find_next_operation(operations, NULL, "__create__", DT_NONE, datatype, DT_NONE);
}


static inline operation_s *
find_destructor(struct array_s *operations, int datatype)
{
    return find_next_operation(operations, NULL, "__destroy__", DT_NONE, datatype, DT_NONE);
}

static inline void
print_operation(FILE *out, operation_s *op)
{
    if (op->arg1_data_type != DT_NONE && op->arg2_data_type != DT_NONE && op->out_data_type != DT_NONE) {
        fprintf(out, "%s (%s, %s) -> %s", op->symbol, data_type_to_str(op->arg1_data_type),
            data_type_to_str(op->arg2_data_type), data_type_to_str(op->out_data_type));
    } else if (op->arg1_data_type != DT_NONE && op->out_data_type != DT_NONE) {
        fprintf(out, "%s (%s) -> %s", op->symbol, data_type_to_str(op->arg1_data_type), data_type_to_str(op->out_data_type));
    } else if (op->arg1_data_type != DT_NONE) {
        fprintf(out, "%s (%s)", op->symbol, data_type_to_str(op->arg1_data_type));
    } else {
        // ??
        fprintf(out, "%s (%s, %s) -> %s", op->symbol, data_type_to_str(op->arg1_data_type),
            data_type_to_str(op->arg2_data_type), data_type_to_str(op->out_data_type));
    }
}


#endif