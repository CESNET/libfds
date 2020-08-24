/**
 * \file src/filter/values.h
 * \author Michal Sedlak <xsedla0v@stud.fit.vutbr.cz>
 * \brief Filter values
 * \date 2020
 */

/* 
 * Copyright (C) 2020 CESNET, z.s.p.o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the Company nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * ALTERNATIVELY, provided that this notice is retained in full, this
 * product may be distributed under the terms of the GNU General Public
 * License (GPL) version 2 or later, in which case the provisions
 * of the GPL apply INSTEAD OF those given above.
 *
 * This software is provided ``as is'', and any express or implied
 * warranties, including, but not limited to, the implied warranties of
 * merchantability and fitness for a particular purpose are disclaimed.
 * In no event shall the company or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 *
 */

#ifndef FDS_FILTER_VALUES_H
#define FDS_FILTER_VALUES_H

#include <stdio.h>
#include <libfds.h>

typedef fds_filter_value_u value_u;

static inline const char *
data_type_to_str(int data_type)
{
    if (data_type & FDS_FDT_CUSTOM) {
        return "unknown custom type";
    }

    switch (data_type) {
    case FDS_FDT_NONE:                 return "none";
    case FDS_FDT_INT:                  return "int";
    case FDS_FDT_UINT:                 return "uint";
    case FDS_FDT_FLOAT:                return "float";
    case FDS_FDT_BOOL:                 return "bool";
    case FDS_FDT_STR:                  return "str";
    case FDS_FDT_IP:                   return "ip";
    case FDS_FDT_MAC:                  return "mac";
    case FDS_FDT_FLAGS:                return "flags";
    case FDS_FDT_LIST | FDS_FDT_NONE:  return "list of none";
    case FDS_FDT_LIST | FDS_FDT_BOOL:  return "list of bool";
    case FDS_FDT_LIST | FDS_FDT_INT:   return "list of int";
    case FDS_FDT_LIST | FDS_FDT_UINT:  return "list of uint";
    case FDS_FDT_LIST | FDS_FDT_FLOAT: return "list of float";
    case FDS_FDT_LIST | FDS_FDT_STR:   return "list of str";
    case FDS_FDT_LIST | FDS_FDT_IP:    return "list of ip";
    case FDS_FDT_LIST | FDS_FDT_MAC:   return "list of mac";
    default:                           return "invalid type";
    }
}

static inline void
print_ipv4_addr(FILE *out, fds_filter_ip_t *ip)
{
    fprintf(out, "%d.%d.%d.%d/%u", ip->addr[0], ip->addr[1], ip->addr[2], ip->addr[3], ip->prefix);
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
print_value(FILE *out, int data_type, fds_filter_value_u *value)
{
    if (data_type & FDS_FDT_LIST) {
        fprintf(out, "[ ");
        for (int i = 0; i < value->list.len; i++) {
            print_value(out, data_type & ~FDS_FDT_LIST, &value->list.items[i]);
            fprintf(out, (i < value->list.len - 1) ? ", " : "");
        }
        fprintf(out, " ]");
        return;
    }

    if (data_type & FDS_FDT_CUSTOM) {
        fprintf(out, "unknown custom value");
        return;
    }

    switch (data_type) {
    case FDS_FDT_INT:   print_int(out, value->i);      break;
    case FDS_FDT_UINT:  print_uint(out, value->u);     break;
    case FDS_FDT_FLOAT: print_float(out, value->f);    break;
    case FDS_FDT_BOOL:  print_bool(out, value->b);     break;
    case FDS_FDT_STR:   print_str(out, &value->str);   break;
    case FDS_FDT_IP:    print_ip(out, &value->ip);     break;
    case FDS_FDT_MAC:   print_mac(out, &value->mac);   break;
    case FDS_FDT_FLAGS:   print_uint(out, value->u);   break;
    default:       fprintf(out, "invalid value"); break;
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
    if (data_type == (FDS_FDT_LIST | FDS_FDT_STR)) {
        for (int i = 0; i < list->len; i++) {
            destroy_str(&list->items[i].str);
        }
    }
    free(list->items);
}

static inline void
destroy_value(int data_type, fds_filter_value_u *value)
{
    if (data_type & FDS_FDT_LIST) {
        destroy_list(data_type, &value->list);
    } else if (data_type == FDS_FDT_STR) {
        destroy_str(value);
    }
}

#endif