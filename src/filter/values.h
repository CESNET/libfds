#ifndef FDS_FILTER_VALUES_H
#define FDS_FILTER_VALUES_H

#include <stdio.h>

#include <libfds.h>

typedef union fds_filter_value_u value_u;
typedef value_u value_t;

// typedef struct fds_filter_ip_s ip_s;
// typedef struct fds_filter_str_s str_s;
// typedef struct fds_filter_list_s list_s;
// typedef struct fds_filter_mac_s mac_s;

// typedef fds_filter_ip_t ip_t;
// typedef fds_filter_str_t str_t;
// typedef fds_filter_list_t list_t;
// typedef fds_filter_mac_t mac_t;
// typedef fds_filter_int_t int_t;
// typedef fds_filter_uint_t uint_t;
// typedef fds_filter_float_t float_t;
// typedef fds_filter_bool_t bool_t;

typedef enum fds_filter_data_type_e data_type_e;
#define DT_ANY FDS_FILTER_DT_ANY
#define DT_NONE FDS_FILTER_DT_NONE
#define DT_INT FDS_FILTER_DT_INT
#define DT_UINT FDS_FILTER_DT_UINT
#define DT_FLOAT FDS_FILTER_DT_FLOAT
#define DT_STR FDS_FILTER_DT_STR
#define DT_BOOL FDS_FILTER_DT_BOOL
#define DT_IP FDS_FILTER_DT_IP
#define DT_MAC FDS_FILTER_DT_MAC
#define DT_CUSTOM FDS_FILTER_DT_CUSTOM
#define DT_LIST FDS_FILTER_DT_LIST

static inline const char *
data_type_to_str(int data_type)
{
    switch (data_type) {
    case DT_NONE:            return "none";
    case DT_INT:             return "int";
    case DT_UINT:            return "uint";
    case DT_FLOAT:           return "float";
    case DT_BOOL:            return "bool";
    case DT_STR:             return "str";
    case DT_IP:              return "ip";
    case DT_MAC:             return "mac";
    case DT_LIST | DT_NONE:  return "list of none";
    case DT_LIST | DT_BOOL:  return "list of bool";
    case DT_LIST | DT_INT:   return "list of int";
    case DT_LIST | DT_UINT:  return "list of uint";
    case DT_LIST | DT_FLOAT: return "list of float";
    case DT_LIST | DT_STR:   return "list of str";
    case DT_LIST | DT_IP:    return "list of ip";
    case DT_LIST | DT_MAC:   return "list of mac";
    default:                 return "unknown";
    }
}

static inline void
print_ipv4_addr(FILE *out, fds_filter_ip_t *ip)
{
    fprintf(out, "%d.%d.%d.%d/%u",
            ip->addr[0], ip->addr[1], ip->addr[2], ip->addr[3],
            ip->prefix);
}

static inline void
print_ipv6_addr(FILE *out, fds_filter_ip_t *ip)
{
    fprintf(out, "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x/%u",
            ip->addr[0], ip->addr[1], ip->addr[2], ip->addr[3],
            ip->addr[4], ip->addr[5], ip->addr[6], ip->addr[7],
            ip->addr[8], ip->addr[9], ip->addr[10], ip->addr[11],
            ip->addr[12], ip->addr[13], ip->addr[14], ip->addr[15],
            ip->prefix);
}

static inline void
print_ip(FILE *out, fds_filter_ip_t *ip)
{
    if (ip->version == 4) {
        print_ipv4_addr(out, ip);
    } else if (ip->version == 6) {
        print_ipv6_addr(out, ip);
    }
}

static inline void
print_mac(FILE *out, fds_filter_mac_t *mac)
{
    fprintf(out, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
            mac->addr[0], mac->addr[1], mac->addr[2],
            mac->addr[3], mac->addr[4], mac->addr[5]);
}

static inline void
print_str(FILE *out, fds_filter_str_t *str)
{
    fputc('"', out);
    for (int i = 0; i < str->len; i++) {
        fputc(str->chars[i], out);
    }
    fputc('"', out);
}

static inline void
print_int(FILE *out, fds_filter_int_t i)
{
    fprintf(out, "%li", i);
}

static inline void
print_uint(FILE *out, fds_filter_uint_t u)
{
    fprintf(out, "%luu", u);
}

static inline void
print_float(FILE *out, fds_filter_float_t f)
{
    fprintf(out, "%lf", f);
}

static inline void
print_bool(FILE *out, fds_filter_bool_t b)
{
    fprintf(out, "%s", b ? "true" : "false");
}

static inline void
print_value(FILE *out, int data_type, value_t *value)
{
    if (data_type & DT_LIST) {
        fprintf(out, "[ ");
        for (int i = 0; i < value->list.len; i++) {
            print_value(out, data_type & ~DT_LIST, &value->list.items[i]);
            fprintf(out, (i < value->list.len - 1) ? ", " : "");
        }
        fprintf(out, " ]");
        return;
    }

    switch (data_type) {
    case DT_INT:   print_int(out, value->i);    break;
    case DT_UINT:  print_uint(out, value->u);   break;
    case DT_FLOAT: print_float(out, value->f);  break;
    case DT_BOOL:  print_bool(out, value->b);   break;
    case DT_STR:   print_str(out, &value->str); break;
    case DT_IP:    print_ip(out, &value->ip);   break;
    case DT_MAC:   print_mac(out, &value->mac); break;
    default:       fprintf(out, "unknown");     break;
    }
}

static inline void
destroy_str(fds_filter_str_t *str)
{
    free(str->chars);
}

static inline void
destroy_list(int data_type, fds_filter_list_t *list)
{
    if (data_type == DT_LIST | DT_STR) {
        for (int i = 0; i < list->len; i++) {
            destroy_str(&list->items[i].str);
        }
    }
    free(list->items);
}

static inline void
destroy_value(int data_type, value_t *value)
{
    if (data_type & DT_LIST) {
        destroy_list(data_type, &value->list);
    } else if (data_type == DT_STR) {
        destroy_str(value);
    }
}

#endif