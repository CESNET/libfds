#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "common.h"
#include "error.h"
#include "scanner.h"

// supported suffixes for number literals
const struct {
    const char *unit;
    uint64_t scale;
} number_units[] = {
    { "B", 1lu },
    { "k", 1024lu },
    { "M", 1024lu * 1024lu },
    { "G", 1024lu * 1024lu * 1024lu },
    { "T", 1024lu * 1024lu * 1024lu * 1024lu },
    { "ns", 1lu },
    { "us", 1000lu },
    { "ms", 1000lu * 1000lu },
    { "s", 1000lu * 1000lu * 1000lu },
    { "m", 60lu * 1000lu * 1000lu * 1000lu },
    { "h", 60lu * 60lu * 1000lu * 1000lu * 1000lu },
    { "d", 24lu * 60lu * 60lu * 1000lu * 1000lu * 1000lu },
};

// strings that are treated as symbols
const char *symbols[] = {
    "~", "not", "*", "/", "+", "-", "|", "&", "^", "%",
    "and", "or", "in", "contains", "exists", "[", "]", "(", ")", ",",
    "<", ">", "==", "!=", ">=", "<=", "<<", ">>",
    "in", "out", "ingress", "egress", "src", "dst"
};

void
print_token(FILE *out, struct token_s *token)
{
    switch (token->kind) {
        case TK_LITERAL:
            fprintf(out, "literal: ");
            fprintf(out, " ");
            print_value(out, token->literal.data_type, &token->literal.value);
            break;
        case TK_NAME:
            fprintf(out, "name: %s", token->name);
            break;
        case TK_SYMBOL:
            fprintf(out, "symbol: %s", token->symbol);
            break;
        case TK_END:
            fprintf(out, "end");
            break;
        case TK_NONE:
            fprintf(out, "none");
            break;
    }
}

static bool
is_bin_digit(char c)
{
    return c == '0' || c == '1';
}

static bool
is_oct_digit(char c)
{
    return c >= '0' && c <= '8'; 
}

static bool
skip_whitespace(const char **cursor)
{
    bool any = false;
    while (isspace(**cursor)) {
        (*cursor)++;
        any = true;
    }
    return any;
}

static bool
scan_ipv4_octet(const char **cursor, uint8_t *out_value, bool *out_overflow)
{
    uint16_t value = 0;
    const char *c = *cursor;
    if (!isdigit(*c)) {
        return false;
    }
    
    value = *c - '0';
    c++;

    if (isdigit(*c)) {
        value *= 10;
        value += *c - '0';
        c++;
    } else {
        goto end;
    }
    if (isdigit(*c)) {
        value *= 10;
        value += *c - '0';
        c++;
    } else {
        goto end;
    }

end:
    *cursor = c;
    *out_value = (uint8_t)value;
    *out_overflow = value > 255;
    return true;
}

static bool
scan_ipv4_address(const char **cursor, struct token_s *out_token, error_t *out_error)
{
    fds_filter_ip_t ip = { .version = 4, .prefix = 32 };
    bool overflow;
    const char *c = *cursor;

    for (int i = 0; i < 4; i++) {
        // Scan the octet
        if (!scan_ipv4_octet(&c, &ip.addr[i], &overflow)) {
            *cursor = c;
            *out_error = LEXICAL_ERROR(c, "invalid octet value in ipv4 address");
            return true;
        }
        if (overflow) {
            *cursor = c;
            *out_error = LEXICAL_ERROR(c, "octet value > 255 in ipv4 address");
            return true;
        }
        // Scan the dot if not last octet
        if (i < 3) {
            if (*c != '.') {
                *cursor = c;
                *out_error = LEXICAL_ERROR(c, "expected . while scanning ipv4 address");
                return true;
            }
            c++;
        }
    }
  
    // Optional prefix
    // TODO: Do we allow whitespace? eg. 192.168.1.0 / 24  
    if (*c != '/') {
        goto end;
    }
    c++;
    // First digit of the prefix, required
    if (!isdigit(*c)) {
        *cursor = c;
        *out_error = LEXICAL_ERROR(c, "expected prefix length after / in ipv4 address");
        return true;
    }
    ip.prefix = *c - '0';
    c++;
    // The following digits if any
    if (!isdigit(*c)) {
        goto end;
    }
    ip.prefix *= 10;
    ip.prefix += *c - '0';
    c++;
    if (ip.prefix > 32 || isdigit(*c)) {
        *cursor = c;
        *out_error = LEXICAL_ERROR(c, "prefix length > 32 in ipv4 address");
        return true;
    }

end:
    *out_error = NO_ERROR;
    struct token_s token = {};
    token.kind = TK_LITERAL;
    token.literal.data_type = DT_IP;
    token.literal.value.ip = ip;
    token.cursor_begin = *cursor;
    token.cursor_end = c;
    *out_token = token;
    *cursor = c;
    return true;    
}

static uint8_t
xdigit_to_number(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return 10 + (c - 'a');
    }
    if (c >= 'A' && c <= 'F') {
        return 10 + (c - 'A');
    }
    ASSERT_UNREACHABLE();
}

static bool
scan_ipv6_hextet(const char **cursor, uint8_t *out_value)
{
    const char *c = *cursor;
    if (!isxdigit(*c)) {
        return false;
    }
    uint16_t value = 0;
    for (int i = 0; i < 4; i++) {
        if (!isxdigit(*c)) {
            goto end;
        }
        value *= 16;
        value += xdigit_to_number(*c);
        c++;
    }
end:
    out_value[0] = (value >> 8) & 0xFF;
    out_value[1] = value & 0xFF;
    *cursor = c;
    return true;
}

static bool
scan_ipv6_address(const char **cursor, struct token_s *out_token, error_t *out_error)
{
    const char *c = *cursor;
    if (!isxdigit(*c) && *c != ':') {
        return false;
    }

    fds_filter_ip_t ip;
    ip.version = 6;
    int double_colon = -1;
    int n_byte = 0;

    // check for a double colon at the beginning
    if (*c == ':' && *(c + 1) == ':') {
        double_colon = 0;
        c += 2;
    }

    for (;;) {
        if (!scan_ipv6_hextet(&c, &ip.addr[n_byte])) {
            if (double_colon != n_byte) {
                // expected hextet, not a valid ipv6 address
                *cursor = c;
                *out_error = LEXICAL_ERROR(c, "expected hextet in ipv6 address");
                return true;
            } else {
                // the address ends with :: 
                break;
            }
        }

        n_byte += 2;
        if (n_byte == 16) {
            // we've reached the end
            break;
        }

        // colon has to follow if its not the last hextet unless it's shortened
        if (*c != ':') {
            if (double_colon == -1) {
                // expected :, not a valid ipv6 address
                *cursor = c;
                *out_error = LEXICAL_ERROR(c, "expected : in ipv6 address");
                return true;
            } else {
                // we've seen a :: somewhere so this is the end of the address
                break;
            }
        }
        c++;

        // check if double colon
        if (*c == ':') {
            // check if it's the first double colon
            if (double_colon != -1) {
                *cursor = c;
                *out_error = LEXICAL_ERROR(c, "multiple :: in ipv6 address");
                return true;
            } else {
                double_colon = n_byte;
                c++;
            }
        }
    }

    // if the address was in the short form correct it
    if (double_colon != -1) {
        memmove(&ip.addr[16 - (n_byte - double_colon)], &ip.addr[double_colon], n_byte - double_colon);
        memset(&ip.addr[double_colon], 0, 16 - n_byte);
    }

    // check for prefix after the address
    uint16_t prefix = 128;
    if (*c != '/') {
        goto end;
    }
    c++;

    if (!isdigit(*c)) {
        *cursor = c;
        *out_error = LEXICAL_ERROR(c, "expected prefix length after / in ipv6 address");
        return true;
    }
    prefix = *c - '0';
    c++;
    
    // The following digits if any
    if (!isdigit(*c)) {
        goto end;
    }
    prefix *= 10;
    prefix += *c - '0';
    c++;

    if (!isdigit(*c)) {
        goto end;
    }
    prefix *= 10;
    prefix += *c - '0';
    c++;

    if (prefix > 128 || isdigit(*c)) {
        *cursor = c;
        *out_error = LEXICAL_ERROR(c, "prefix length > 128 in ipv6 address");
        return true;
    }

end:
    ip.prefix = (uint8_t)prefix;
    struct token_s token = {};
    token.kind = TK_LITERAL;
    token.literal.data_type = DT_IP;
    token.literal.value.ip = ip;
    token.cursor_begin = *cursor;
    token.cursor_end = c;
    *cursor = c;
    *out_token = token;
    *out_error = NO_ERROR;
    return true;
}

static bool
scan_mac_address(const char **cursor, struct token_s *out_token, error_t *out_error)
{
    const char *c = *cursor;
    if (!isxdigit(*c)) {
        return false;
    }

    fds_filter_mac_t mac;

    if (!isxdigit(c[0]) || !isxdigit(c[1])) { goto fail_digit; }
    mac.addr[0] = xdigit_to_number(c[0]) * 16 + xdigit_to_number(c[1]);
    c += 2;
    if (*c != ':') { goto fail_semicolon; }
    c++;

    if (!isxdigit(c[0]) || !isxdigit(c[1])) { goto fail_digit; }
    mac.addr[1] = xdigit_to_number(c[0]) * 16 + xdigit_to_number(c[1]);
    c += 2;
    if (*c != ':') { goto fail_semicolon; }
    c++;

    if (!isxdigit(c[0]) || !isxdigit(c[1])) { goto fail_digit; }
    mac.addr[2] = xdigit_to_number(c[0]) * 16 + xdigit_to_number(c[1]);
    c += 2;
    if (*c != ':') { goto fail_semicolon; }
    c++;

    if (!isxdigit(c[0]) || !isxdigit(c[1])) { goto fail_digit; }
    mac.addr[3] = xdigit_to_number(c[0]) * 16 + xdigit_to_number(c[1]);
    c += 2;
    if (*c != ':') { goto fail_semicolon; }
    c++;

    if (!isxdigit(c[0]) || !isxdigit(c[1])) { goto fail_digit; }
    mac.addr[4] = xdigit_to_number(c[0]) * 16 + xdigit_to_number(c[1]);
    c += 2;
    if (*c != ':') { goto fail_semicolon; }
    c++;

    if (!isxdigit(c[0]) || !isxdigit(c[1])) { goto fail_digit; }
    mac.addr[5] = xdigit_to_number(c[0]) * 16 + xdigit_to_number(c[1]);
    c += 2;

    struct token_s token = {};
    token.kind = TK_LITERAL;
    token.literal.data_type = DT_MAC;
    token.literal.value.mac = mac;
    token.cursor_begin = *cursor;
    token.cursor_end = c;
    *cursor = c;
    *out_token = token;
    *out_error = NO_ERROR;
    return true;

fail_semicolon:
    *cursor = c;
    *out_error = LEXICAL_ERROR(c, "expected : in mac address");
    return true;

fail_digit:
    *cursor = c;
    *out_error = LEXICAL_ERROR(c, "expected hex in mac address");
    return true;
}

static bool
scan_string(const char **cursor, struct token_s *out_token, error_t *out_error)
{
    fds_filter_str_t str = { };
    const char *c = *cursor;
    if (*c != '"') {
        return false;
    }
    c++;
    // Check if empty string so we don't unnecessarily allocate
    if (*c == '"') {
        c++;
        goto end;
    }

    int idx = 0;
    int capacity = 32; // Arbitary starting capacity
    str.chars = malloc(capacity);
    if (!str.chars) { 
        *out_error = MEMORY_ERROR;
        return true;
    }
    str.len = 0;
    while (*c != '"') {
        if (*c == '\0') {
            free(str.chars);
            *cursor = c;
            *out_error = LEXICAL_ERROR(c, "unterminated string");
            return true;
        }
        
        if (str.len == capacity) {
            capacity *= 2;
            void *tmp = realloc(str.chars, capacity);
            if (!tmp) {
                free(str.chars);
                *out_error = MEMORY_ERROR;
                return true;
            }
            str.chars = tmp;
        }

        if (*c == '\\') {
            c++;
            if (*c == '\0') {
                free(str.chars);
                *cursor = c;
                *out_error = LEXICAL_ERROR(c, "unterminated string");
                return true;
            }

            if (*c == 't') {
                str.chars[str.len] = '\t';
                str.len++;
                c++;
            } else if (*c == 'n') {
                str.chars[str.len] = '\n';
                str.len++;
                c++;
            } else if (*c == 'r') {
                str.chars[str.len] = '\r';
                str.len++;
                c++;
            } else if (*c == '"') {
                str.chars[str.len] = '"';
                str.len++;
                c++;
            } else if (is_oct_digit(c[0]) && is_oct_digit(c[1]) && is_oct_digit(c[2])) {
                // 3 octal digits - eg. \042
                str.chars[str.len] = (c[0] - '0') * 8 * 8 + (c[1] - '0') * 8 + (c[2] - '0');
                str.len++;
                c += 3;
            } else if (c[0] == 'x' && isxdigit(c[1]) && isxdigit(c[2])) {
                // 2 hex digits prefixed with x - eg. \xff
                str.chars[str.len] = xdigit_to_number(c[1]) * 16 + xdigit_to_number(c[2]);
                str.len++;
                c += 3;
            } else if (*c == '\\') {
                str.chars[str.len] = '\\';
                str.len++;
                c++;
            } else {
                // If it's unknown escape sequence treat it as if it was raw string
                str.chars[str.len] = '\\';
                str.len++;
            }
        } else {
            str.chars[str.len] = *c; 
            str.len++;
            c++;
        }
    }
    c++;

end:
    *out_error = NO_ERROR;
    struct token_s token = {};
    token.kind = TK_LITERAL;
    token.literal.data_type = DT_STR;
    token.literal.value.str = str;
    token.cursor_begin = *cursor;
    token.cursor_end = c;
    *out_token = token;
    *cursor = c;
    return true;
}

static bool
scan_symbol(const char **cursor, struct token_s *out_token, error_t *out_error)
{
    // Find the first longest symbol that matches at the cursor
    const char *best_symbol = NULL;
    for (int i = 0; i < CONST_ARR_SIZE(symbols); i++) {
        if (strncmp(symbols[i], *cursor, strlen(symbols[i])) == 0) {
            if (!best_symbol || strlen(best_symbol) < strlen(symbols[i])) {
                best_symbol = symbols[i];
            }
        }
    }
    if (!best_symbol) {
        return false;
    }
    struct token_s token = {};
    token.kind = TK_SYMBOL;
    token.symbol = best_symbol;
    token.cursor_begin = *cursor;
    token.cursor_end = *cursor + strlen(best_symbol);
    *out_error = NO_ERROR;
    *out_token = token;
    *cursor += strlen(best_symbol);
    return true;
}

static bool
scan_name(const char **cursor, struct token_s *out_token, error_t *out_error)
{
    const char *c = *cursor;
    
    if (!isalpha(*c)) {
        return false;
    }
    c++;
    while (*c && (isalnum(*c) || strchr(":@-._", *c))) {
        c++;
    }
    
    int name_len = c - *cursor;
    char *name = malloc(name_len + 1);
    if (!name) {
        *out_error = MEMORY_ERROR;
        return true;
    }
    strncpy(name, *cursor, name_len);
    name[name_len] = '\0';

    struct token_s token = {};
    token.kind = TK_NAME;
    token.name = name;
    token.cursor_begin = *cursor;
    token.cursor_end = c;
    *cursor = c;
    *out_token = token;
    *out_error = NO_ERROR;
    return true;
}

static bool
scan_number(const char **cursor, struct token_s *out_token, error_t *out_error)
{
    const char *c = *cursor;
    struct token_s token = {};

    // Hex literal
    if (strncmp(c, "0x", 2) == 0) {
        c += 2;
        if (!isxdigit(*c)) {
            *cursor = c;
            *out_error = LEXICAL_ERROR(c, "expected hex digit while scanning hex literal");
            return true;
        }
        uint64_t value = 0;
        while (isxdigit(*c)) {
            // TODO: check for overflow?
            value *= 16;
            value += xdigit_to_number(*c);
            c++;
        }
        token.kind = TK_LITERAL;
        token.literal.data_type = DT_INT;
        token.literal.value.u = value;
        token.cursor_begin = *cursor;
        token.cursor_end = c;
        *cursor = c;
        *out_token = token;
        *out_error = NO_ERROR;
        return true;

    // Bin literal
    } else if (strncmp(c, "0b", 2) == 0) {
        c += 2;
        if (!is_bin_digit(*c)) {
            *cursor = c;
            *out_error = LEXICAL_ERROR(c, "expected bin digit while scanning bin literal");
            return true;
        }
        uint64_t value = 0;
        while (is_bin_digit(*c)) {
            // TODO: check for overflow?
            value *= 2;
            value += *c - '0';
            c++;
        }
        token.kind = TK_LITERAL;
        token.literal.data_type = DT_INT;
        token.literal.value.u = value;
        token.cursor_begin = *cursor;
        token.cursor_end = c;
        *cursor = c;
        *out_token = token;
        *out_error = NO_ERROR;
        return true;
    
    // Number literal
    } else {
        bool is_float = false;
        bool any_digit = false;

        uint64_t value = 0;
        while (isdigit(*c)) {
            any_digit = true;
            value *= 10;
            value += *c - '0';
            c++;
        }

        // The part after decimal point
        double dp_value = 0;
        if (*c == '.') {
            is_float = true;
            double dp_div = 10;
            c++;
            while (isdigit(*c)) {
                any_digit = true;
                dp_value += (*c - '0') / dp_div; 
                dp_div *= 10;
                c++;
            }
        }

        // If there was no digit it's not a number match
        if (!any_digit) {
            return false;
        }

        // Exponent if any
        double exp = 0;
        int exp_sign = 1;
        if (*c == 'e' || *c == 'E') {
            is_float = true;
            c++;
            if (*c == '+' || *c == '-') {
                if (*c == '-') {
                    exp_sign = -1;
                }
                c++;
            }
            if (!isdigit(*c)) {
                *cursor = c;
                *out_error = LEXICAL_ERROR(c, "expected digit in exponent");
                return true;
            }
            while (isdigit(*c)) {
                exp *= 10;
                exp += *c - '0';
                c++;
            }
        }
        exp *= exp_sign;

        // Number unit
        double scale = 1;
        for (int i = 0; i < CONST_ARR_SIZE(number_units); i++) {
            if (strncmp(c, number_units[i].unit, strlen(number_units[i].unit)) == 0) {
                scale = number_units[i].scale;
                if (scale != floor(scale)) {
                    is_float = true;
                }
                c += strlen(number_units[i].unit);
            }
        }

        // Unsigned specifier suffix
        bool is_unsigned = false;
        if (*c == 'u' || *c == 'U') {
            if (is_float) {
                *cursor = c;
                *out_error = LEXICAL_ERROR(c, "float cannot be unsigned");
                return true;
            }
            is_unsigned = true;
            c++;
        }

        assert(!is_unsigned || !is_float);
        token.kind = TK_LITERAL;
        token.cursor_begin = *cursor;
        token.cursor_end = c;
        if (is_unsigned) {
            token.literal.data_type = DT_UINT;
            // TODO: check overflow?
            token.literal.value.u = value * (uint64_t)scale;
        } else if (is_float) {
            token.literal.data_type = DT_FLOAT;
            token.literal.value.f = ((double)value + dp_value) * pow(10, exp) * scale;
        } else {
            token.literal.data_type = DT_INT;
            // TODO: check overflow?
            token.literal.value.i = value * (int64_t)scale;
        }
        *cursor = c;
        *out_token = token;
        *out_error = NO_ERROR;
        return true;
    }
}

static bool
scan_decimal(const char **cursor, uint64_t *out_number)
{
    uint64_t number = 0;
    const char *c = *cursor;
    if (!isdigit(*c)) {
        return false;
    }
    while (isdigit(*c)) {
        number *= 10;
        number += *c - '0';
        c++;
    }
    *cursor = c;
    *out_number = number;
    return true;
}

static bool
scan_datetime(const char **cursor, struct token_s *out_token, error_t *out_error)
{
    const char *start_cursor = *cursor;

    uint64_t year = 0;
    uint64_t month = 0;
    uint64_t day = 0;
    uint64_t hour = 0;
    uint64_t min = 0;
    uint64_t sec = 0;
    int offset_sign = 1;
    uint64_t offset_hour = 0;
    uint64_t offset_min = 0;
    bool is_localtime = false;

    // year
    if (!scan_decimal(cursor, &year)) {
        // expected year
        return false;
    }
    // -
    if (**cursor != '-') {
        // expected -
        return false;
    }
    (*cursor)++;
    // month
    if (!scan_decimal(cursor, &month)) {
        // expected month
        return false;
    }
    // -
    if (**cursor != '-') {
        // expected -
        return false;
    }
    (*cursor)++;
    // day
    if (!scan_decimal(cursor, &day)) {
        // expected day
        return false;
    }
    // optional T
    if (**cursor == 'T') {
        (*cursor)++;
        // hour
        if (!scan_decimal(cursor, &hour)) {
            // expected hour
            return false;
        }
        // :
        if (**cursor != ':') {
            // expected :
            return false;
        }
        (*cursor)++;
        // minute
        if (!scan_decimal(cursor, &min)) {
            // expected minute
            return false;
        }
        // optional :second
        if (**cursor == ':') {
            (*cursor)++;
            // second
            if (!scan_decimal(cursor, &sec)) {
                // expected second
                return false;
            }
        }
    }
    // Z or +- offset
    if (**cursor == 'Z') {
        // utc
        (*cursor)++;
    } else if (**cursor == '+' || **cursor == '-') {
        if (**cursor == '-') {
            offset_sign = -1;
        }
        (*cursor)++;
        // hour
        if (!scan_decimal(cursor, &offset_hour)) {
            // expected offset hour
            return false;
        }
        // optional :minute
        if (**cursor == ':') {
            if (!scan_decimal(cursor, &offset_min)) {
                // expected offset minute
                return false;
            }
        }
    } else {
        // TODO: local tz?
        is_localtime = true;
    }

end: ;
    struct tm t = {
        .tm_year = year,
        .tm_mon = month,
        .tm_mday = day,
        .tm_hour = hour,
        .tm_min = min,
        .tm_sec = sec,
    };
    time_t epoch_secs = is_localtime ? timelocal(&t) : timegm(&t); // XXXX: GNU/BSD extension
    if (epoch_secs == (time_t)-1) {
        *out_error = LEXICAL_ERROR(*cursor, "invalid datetime");
        return true;
    }

    if (offset_sign == 1) {
        epoch_secs += offset_hour * 3600 + offset_min * 60;
    } else if (offset_sign == -1) {
        epoch_secs -= offset_hour * 3600 + offset_min * 60;
    }

    // TODO: check overflow
    uint64_t epoch_nanosecs = epoch_secs * 1000000000;

    struct token_s token;
    token.kind = TK_LITERAL;
    token.literal.data_type = DT_UINT;
    token.literal.value.u = epoch_nanosecs;
    token.cursor_begin = start_cursor;
    token.cursor_end = *cursor;
    *out_error = NO_ERROR;
    *out_token = token;
    return true;
}

static bool
scan_bool(const char **cursor, struct token_s *out_token, error_t *out_error)
{
    struct token_s token = {};
    *out_error = NO_ERROR;
    token.kind = TK_LITERAL;
    token.literal.data_type = DT_BOOL;
    token.cursor_begin = *cursor;
    if (strncmp(*cursor, "true", 4) == 0) {
        *cursor += 4;
        token.cursor_end = *cursor;
        token.literal.value.b = true;
        *out_token = token;
        return true;
    } else if (strncmp(*cursor, "false", 5) == 0) {
        *cursor += 5;
        token.cursor_end = *cursor;
        token.literal.value.b = false;
        *out_token = token;
        return true;
    }
    return false;
}

typedef bool (scan_func_t)(const char **cursor, struct token_s *out_token, error_t *out_error);

const scan_func_t *scan_funcs[] = {
    scan_symbol,
    scan_ipv4_address,
    scan_ipv6_address,
    scan_mac_address,
    scan_datetime,
    scan_number,
    scan_string,
    scan_bool,
    scan_name,
};

static bool
scan_token(const char **cursor, struct token_s *out_token, error_t *out_best_error)
{
    // TODO: this function is a terrible mess

    // End of input
    if (**cursor == '\0') {
        struct token_s token = {};
        token.kind = TK_END;
        token.cursor_begin = *cursor;
        token.cursor_end = *cursor;
        *out_token = token;
        *out_best_error = NO_ERROR;
        return true;
    }

    struct token_s best_token = {};
    error_t best_err = NO_ERROR;
    const char *best_err_cursor = 0;
    bool any_match = false;
    bool any_success = false;
    for (int i = 0; i < CONST_ARR_SIZE(scan_funcs); i++) {
        const char *c = *cursor;
        struct token_s token;
        error_t err;
        bool match = scan_funcs[i](&c, &token, &err);
        if (!match) {
            continue;
        }
        any_match = true;

        if (err == NO_ERROR) {
            if (any_success) {
                if (token.cursor_end - token.cursor_begin > best_token.cursor_end - best_token.cursor_begin) {
                    destroy_token(best_token);
                    best_token = token;
                } else {
                    destroy_token(token);
                }
            } else {
                best_token = token;
                any_success = true;        
            }
        } else {
            if (best_err == NO_ERROR) {
                best_err = err;
                best_err_cursor = c;
            } else {
                if (c > best_err_cursor) {
                    destroy_error(best_err);
                    best_err = err;
                    best_err_cursor = c;
                } else {
                    destroy_error(err);
                }
            }
        }
    }

    if (any_success) {
        *cursor = best_token.cursor_end;
        *out_token = best_token;
        *out_best_error = best_err;
        return true;
    } else if (any_match) {
        *out_best_error = best_err;
        return false;
    } else {
        *out_best_error = NO_ERROR;
        return false;
    }
}

static bool
is_not_word_symbol(const char *s)
{
    while (*s) {
        if (isalnum(*s)) {
            return false;
        }
        s++;
    }
    return true;
}

static bool
whitespace_or_end_follows(struct token_s *token)
{
    return isspace(token->cursor_end) || token->cursor_end == '\0';
}


// Scan a new token if token is not ready, else do nothing
error_t
next_token(struct scanner_s *scanner, struct token_s *out_token)
{
    if (scanner->token_ready) {
        *out_token = scanner->token;
        return NO_ERROR;
    }

    const char *cursor = scanner->cursor;
    struct token_s token, next_token;
    error_t err, next_err;
    skip_whitespace(&cursor);
    if (!scan_token(&cursor, &token, &err)) {
        if (err != NO_ERROR) {
            return err;
        } else {
            return LEXICAL_ERROR(cursor, "invalid syntax");
        }
    }

    skip_whitespace(&cursor);
    if (!scan_token(&cursor, &next_token, &next_err)) {
        destroy_error(next_err);
        destroy_token(token);
        if (err != NO_ERROR) {
            return err;
        } else {
            return LEXICAL_ERROR(cursor, "invalid syntax");
        }
    }

    // Valid cases:
    // <any token><whitespace><any token>
    // <any token><no whitespace><symbol without letters>
    // <symbol without letters><no whitespace><any token>
    // <any token><end of input>
    bool valid_token_pair = 
        (isspace(*token.cursor_end) || *token.cursor_end == '\0') // Whitespace or end follows the first token
        || (token.kind == TK_SYMBOL && is_not_word_symbol(token.symbol)) // First token is a non-word symbol
        || (next_token.kind == TK_SYMBOL && is_not_word_symbol(next_token.symbol)); // Second token is a non-word symbol
    if (!valid_token_pair) {
        if (err != NO_ERROR) {
            destroy_error(next_err);
            destroy_token(token);
            destroy_token(next_token);
            return err;
        } else {
            error_t ret_err = LEXICAL_ERROR(token.cursor_end, "invalid syntax");
            destroy_error(err);
            destroy_error(next_err);
            destroy_token(token);
            destroy_token(next_token);
            return ret_err;
        }
    }
    scanner->token = token;
    scanner->token_ready = true;
    scanner->cursor = token.cursor_end;
    *out_token = token;
    destroy_error(err);
    destroy_error(next_err);
    destroy_token(next_token);
    #ifdef FDS_FILTER_DEBUG_SCANNER
    fprintf(stderr, "scanned token: ");
    print_token(stderr, &token);
    fprintf(stderr, "\n");
    #endif
    return NO_ERROR;
}

void
consume_token(struct scanner_s *scanner)
{
    assert(scanner->token_ready);
    scanner->token_ready = false;
}

bool
token_is(struct token_s token, enum token_kind_e kind)
{
    return token.kind == kind;
}

bool
token_is_symbol(struct token_s token, const char *symbol)
{
    return token.kind == TK_SYMBOL && strcmp(symbol, token.symbol) == 0;
}

void
destroy_token(struct token_s token)
{
    if (token.kind == TK_LITERAL && token.literal.data_type == DT_STR) {
        free(token.literal.value.str.chars);
    } else if (token.kind == TK_NAME) {
        free(token.name);
    }
}

void
init_scanner(struct scanner_s *scanner, const char *input)
{
    *scanner = (struct scanner_s) {
        .input = input,
        .cursor = input
    };
    #ifdef FDS_FILTER_DEBUG_SCANNER
    fprintf(stderr, "scanner initialized with input: %s\n", input);
    #endif
}