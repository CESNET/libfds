#ifndef FDS_FILTER_DEBUG_H
#define FDS_FILTER_DEBUG_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <libfds.h>
#include <assert.h>
#include "filter.h"
#include "evaluator.h"
#include "evaluator_functions.h"

#ifndef NDEBUG
#define PTRACE(fmt, ...) \
    do { fprintf(stderr, "%s:%d:%s(): " fmt "\n", __FILE__, __LINE__, __func__,##__VA_ARGS__); } while (0)
#define PDEBUG(fmt, ...) \
    do { fprintf(stderr, fmt "\n",##__VA_ARGS__); } while (0)
#else
#define PTRACE(fmt, ...) /* nothing */
#define PDEBUG(fmt, ...) /* nothing */
#endif

#define RETURN_IF_ERROR(x) \
{ \
    int return_code = x; \
    if (return_code != FDS_FILTER_OK) { \
        PTRACE("propagating failure"); \
        return return_code; \
    } \
}

static inline const char *
ast_node_type_to_str(enum fds_filter_ast_node_type type)
{
    switch (type) {
    case FDS_FANT_NONE:       return "NONE";
    case FDS_FANT_ADD:        return "ADD";
    case FDS_FANT_MUL:        return "MUL";
    case FDS_FANT_SUB:        return "SUB";
    case FDS_FANT_DIV:        return "DIV";
    case FDS_FANT_MOD:        return "MOD";
    case FDS_FANT_UMINUS:     return "UMINUS";
    case FDS_FANT_BITNOT:     return "BITNOT";
    case FDS_FANT_BITAND:     return "BITAND";
    case FDS_FANT_BITOR:      return "BITOR";
    case FDS_FANT_BITXOR:     return "BITXOR";
    case FDS_FANT_FLAGCMP:    return "FLAGCMP";
    case FDS_FANT_NOT:        return "NOT";
    case FDS_FANT_AND:        return "AND";
    case FDS_FANT_OR:         return "OR";
    case FDS_FANT_IMPLICIT:   return "IMPLICIT";
    case FDS_FANT_EQ:         return "EQ";
    case FDS_FANT_NE:         return "NE";
    case FDS_FANT_GT:         return "GT";
    case FDS_FANT_LT:         return "LT";
    case FDS_FANT_GE:         return "GE";
    case FDS_FANT_LE:         return "LE";
    case FDS_FANT_CONST:      return "CONST";
    case FDS_FANT_IDENTIFIER: return "IDENTIFIER";
    case FDS_FANT_LIST:       return "LIST";
    case FDS_FANT_LIST_ITEM:  return "LIST_ITEM";
    case FDS_FANT_IN:         return "IN";
    case FDS_FANT_CONTAINS:   return "CONTAINS";
    case FDS_FANT_CAST:       return "CAST";
    case FDS_FANT_ANY:        return "ANY";
    case FDS_FANT_ROOT:       return "ROOT";
    default:                  assert(!"unhandled ast node type case");
    }
}

static inline const char *
data_type_to_str(enum fds_filter_data_type type)
{
    switch (type) {
    case FDS_FDT_NONE:          return "NONE";
    case FDS_FDT_STR:           return "STR";
    case FDS_FDT_UINT:          return "UINT";
    case FDS_FDT_INT:           return "INT";
    case FDS_FDT_FLOAT:         return "FLOAT";
    case FDS_FDT_BOOL:          return "BOOL";
    case FDS_FDT_IP_ADDRESS:    return "IP_ADDRESS";
    case FDS_FDT_MAC_ADDRESS:   return "MAC_ADDRESS";
    case FDS_FDT_LIST:          return "LIST";
    default:                    assert("unhandled data type case");
    }
    return "<unknown>";
}

static inline const char *
evaluate_function_to_str(void (*function)(struct fds_filter *filter, struct eval_node *node))
{
    if (function == f_add_uint)                return "ADD_UINT";
    if (function == f_sub_uint)                return "SUB_UINT";
    if (function == f_mul_uint)                return "MUL_UINT";
    if (function == f_div_uint)                return "DIV_UINT";
    if (function == f_eq_uint)                 return "EQ_UINT";
    if (function == f_ne_uint)                 return "NE_UINT";
    if (function == f_lt_uint)                 return "LT_UINT";
    if (function == f_gt_uint)                 return "GT_UINT";
    if (function == f_le_uint)                 return "LE_UINT";
    if (function == f_ge_uint)                 return "GE_UINT";
    if (function == f_cast_uint_to_float)      return "CAST_UINT_TO_FLOAT";
    if (function == f_cast_uint_to_bool)       return "CAST_UINT_TO_BOOL";
    if (function == f_add_int)                 return "ADD_INT";
    if (function == f_sub_int)                 return "SUB_INT";
    if (function == f_mul_int)                 return "MUL_INT";
    if (function == f_div_int)                 return "DIV_INT";
    if (function == f_eq_int)                  return "EQ_INT";
    if (function == f_ne_int)                  return "NE_INT";
    if (function == f_lt_int)                  return "LT_INT";
    if (function == f_gt_int)                  return "GT_INT";
    if (function == f_le_int)                  return "LE_INT";
    if (function == f_ge_int)                  return "GE_INT";
    if (function == f_minus_int)               return "MINUS_INT";
    if (function == f_cast_int_to_uint)        return "CAST_INT_TO_UINT";
    if (function == f_cast_int_to_float)       return "CAST_INT_TO_FLOAT";
    if (function == f_cast_int_to_bool)        return "CAST_INT_TO_BOOL";
    if (function == f_add_float)               return "ADD_FLOAT";
    if (function == f_sub_float)               return "SUB_FLOAT";
    if (function == f_mul_float)               return "MUL_FLOAT";
    if (function == f_div_float)               return "DIV_FLOAT";
    if (function == f_eq_float)                return "EQ_FLOAT";
    if (function == f_ne_float)                return "NE_FLOAT";
    if (function == f_lt_float)                return "LT_FLOAT";
    if (function == f_gt_float)                return "GT_FLOAT";
    if (function == f_le_float)                return "LE_FLOAT";
    if (function == f_ge_float)                return "GE_FLOAT";
    if (function == f_minus_float)             return "MINUS_FLOAT";
    if (function == f_cast_float_to_bool)      return "CAST_FLOAT_TO_BOOL";
    if (function == f_concat_str)              return "CONCAT_STR";
    if (function == f_eq_str)                  return "EQ_STR";
    if (function == f_ne_str)                  return "NE_STR";
    if (function == f_cast_str_to_bool)        return "CAST_STR_TO_BOOL";
    if (function == f_eq_ip_address)           return "EQ_IP_ADDRESS";
    if (function == f_ne_ip_address)           return "NE_IP_ADDRESS";
    if (function == f_eq_mac_address)          return "EQ_MAC_ADDRESS";
    if (function == f_ne_mac_address)          return "NE_MAC_ADDRESS";
    if (function == f_and)                     return "AND";
    if (function == f_or)                      return "OR";
    if (function == f_not)                     return "NOT";
    if (function == f_const)                   return "CONST";
    if (function == f_identifier)              return "IDENTIFIER";
    if (function == f_any)                     return "ANY";
    if (function == f_exists)                  return "EXISTS";
    if (function == f_in_uint)                 return "IN_UINT";
    if (function == f_in_int)                  return "IN_INT";
    if (function == f_in_float)                return "IN_FLOAT";
    if (function == f_in_str)                  return "IN_STR";
    if (function == f_in_ip_address)           return "IN_IP_ADDRESS";
    if (function == f_in_mac_address)          return "IN_MAC_ADDRESS";
    if (function == f_ip_address_in_trie)      return "IP_ADDRESS_IN_TRIE";
    if (function == f_bitand)                  return "BITAND";
    if (function == f_bitor)                   return "BITOR";
    if (function == f_bitxor)                  return "BITXOR";
    if (function == f_bitnot)                  return "BITNOT";
    if (function == f_flagcmp)                 return "FLAGCMP";
    if (function == f_mod_int)                 return "MOD_INT";
    if (function == f_mod_uint)                return "MOD_UINT";
    if (function == f_mod_float)               return "MOD_FLOAT";
    if (function == f_cast_list_uint_to_float) return "CAST_LIST_UINT_TO_FLOAT";
    if (function == f_cast_list_int_to_uint)   return "CAST_LIST_INT_TO_UINT";
    if (function == f_cast_list_int_to_float)  return "CAST_LIST_INT_TO_FLOAT";
    else                                       assert(!"unhandled evaluate function case");
}

static inline void
print_value(FILE *output,
    enum fds_filter_data_type type, enum fds_filter_data_type subtype,
    const union fds_filter_value value)
{
    switch (type) {

    case FDS_FDT_NONE: {
        fprintf(output, "<none>");
    } break;

    case FDS_FDT_BOOL: {
        fprintf(output, value.bool_ ? "true" : "false");
    } break;

    case FDS_FDT_INT: {
        fprintf(output, "%li", value.int_);
    } break;

    case FDS_FDT_UINT: {
        fprintf(output, "%luu", value.uint_);
    } break;

    case FDS_FDT_FLOAT: {
        fprintf(output, "%lf", value.float_);
    } break;

    case FDS_FDT_MAC_ADDRESS: {
        const uint8_t *b = value.mac_address;
        fprintf(output, "%02x:%02x:%02x:%02x:%02x:%02x", b[0], b[1], b[2], b[4], b[5], b[6]);
    } break;

    case FDS_FDT_IP_ADDRESS: {
        const uint8_t *b = value.ip_address.bytes;
        if (value.ip_address.version == 4) {
            fprintf(output, "%d.%d.%d.%d", b[0], b[1], b[2], b[3]);
            fprintf(output, "/%d", value.ip_address.prefix_length);
        } else if (value.ip_address.version == 6) {
            fprintf(output, "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
                b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], b[8], b[9], b[10], b[11], b[12], b[13], b[14], b[15]);
            fprintf(output, "/%d", value.ip_address.prefix_length);
        } else {
            fprintf(output, "<invalid ip address value>");
        }
    } break;

    case FDS_FDT_STR: {
        assert(value.string.length <= INT_MAX);
        fprintf(output, "\"%*s\"", (int)value.string.length, value.string.chars);
    } break;

    case FDS_FDT_LIST: {
        fprintf(output, "[ ");
        if (value.list.items != NULL) {
            for (uint64_t i = 0; i < value.list.length; i++) {
                if (i > 0) {
                    fprintf(output, ", ");
                }
                print_value(output, subtype, FDS_FDT_NONE, value.list.items[i]);
            }
        }
        fprintf(output, " ]");
    } break;

    default: {
        assert(!"unhandled print case");
    };

    }
}

static inline void
print_ast_node(FILE *output, struct fds_filter_ast_node *node)
{
    fprintf(output, "(");
    fprintf(output, "%s ", ast_node_type_to_str(node->node_type));
    if (node->is_trie) {
        fprintf(output, "<optimized to trie>");
    } else {
        print_value(output, node->data_type, node->data_subtype, node->value);
    }
    fprintf(output, ")");
}

static inline void
print_ast_one(FILE *output, struct fds_filter_ast_node *node, int indent_level)
{
    if (node == NULL) {
        return;
    }

    for (int i = 0; i < indent_level; i++) {
        fprintf(output, "    ");
    }
    print_ast_node(output, node);
    fprintf(output, "\n");

    print_ast_one(output, node->left, indent_level + 1);
    print_ast_one(output, node->right, indent_level + 1);
}

static inline void
print_ast(FILE *output, struct fds_filter_ast_node *root_node)
{
    print_ast_one(output, root_node, 0);
}

static inline void
print_eval_node(FILE *output, struct eval_node *node)
{
    fprintf(output, "(");
    fprintf(output, "%s ", evaluate_function_to_str(node->evaluate));
    if (!node->is_defined) {
        fprintf(output, "<undefined>");
    } else if (node->is_trie) {
        fprintf(output, "<optimized to trie>");
    } else {
        print_value(output, node->data_type, node->data_subtype, node->value);
    }
    fprintf(output, ")");
}

static inline void
print_eval_tree_one(FILE *output, struct eval_node *node, int indent_level)
{
    if (node == NULL) {
        return;
    }

    for (int i = 0; i < indent_level; i++) {
        fprintf(output, "    ");
    }
    print_eval_node(output, node);
    fprintf(output, "\n");

    print_eval_tree_one(output, node->left, indent_level + 1);
    print_eval_tree_one(output, node->right, indent_level + 1);
}

static inline void
print_eval_tree(FILE *output, struct eval_node *root_node)
{
    print_eval_tree_one(output, root_node, 0);
}

#endif // FDS_FILTER_DEBUG_H