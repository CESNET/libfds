#ifndef FDS_FILTER_EVALUATE_H
#define FDS_FILTER_EVALUATE_H

#include "api.h"
#include "filter.h"

enum opcode {
    NONE = 0,

    LOAD_CONST,
    LOAD_IDENTIFIER,
    JUMP_IF_MORE,
    JUMP_IF_TRUE,
    JUMP_IF_FALSE,
    
    ADD_UINT,
    SUB_UINT,
    MUL_UINT,
    DIV_UINT,
    CAST_UINT_TO_INT,
    CAST_UINT_TO_FLOAT,
    CMP_EQ_UINT,
    CMP_NE_UINT,
    CMP_GT_UINT,
    CMP_LT_UINT,
    CMP_GE_UINT,
    CMP_LE_UINT,
    
    ADD_INT,
    SUB_INT,
    MUL_INT,
    DIV_INT,
    NEGATE_INT,
    CAST_INT_TO_FLOAT,
    CMP_EQ_INT,
    CMP_NE_INT,
    CMP_GT_INT,
    CMP_LT_INT,
    CMP_GE_INT,
    CMP_LE_INT,
    CMP_EQ_INT,

    ADD_FLOAT,
    SUB_FLOAT,
    MUL_FLOAT,
    DIV_FLOAT,
    NEGATE_FLOAT,
    CMP_EQ_FLOAT,
    CMP_NE_FLOAT,
    CMP_GT_FLOAT,
    CMP_LT_FLOAT,
    CMP_GE_FLOAT,
    CMP_LE_FLOAT,

    CMP_EQ_IP_ADDR,
    CMP_NE_IP_ADDR,

    CMP_EQ_MAC_ADDR,
    CMP_NE_MAC_ADDR,

    STR_CONTAINS_STR,
    CONCAT_STR,
    CMP_EQ_STR,
    CMP_NE_STR,

    LIST_CONTAINS_STR,
    LIST_CONTAINS_INT,
    LIST_CONTAINS_UINT,
    LIST_CONTAINS_FLOAT,
    LIST_CONTAINS_IP_ADDR,
    LIST_CONTAINS_MAC_ADDR,
};

struct instruction {
    int opcode;
    union fds_filter_value arg;
};

struct evaluate_context {
    int program_counter;
    
    int stack_top;
    int stack_capacity;
    union fds_filter_value *stack;

    int instructions_count;
    struct instruction *instructions;
};

#endif // FDS_FILTER_EVALUATE_H