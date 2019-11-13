/**
 * \file src/converters/converters.c
 * \author Lukas Hutak <lukas.hutak@cesnet.cz>
 * \author Pavel Yadlouski <xyadlo00@stud.fit.vutbr.cz>
 * \brief Conversion of IPFIX Data Record to JSON  (source code)
 * \date May 2019
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2019 CESNET z.s.p.o.
 */

#include <libfds.h>
#include <math.h>
#include <stdlib.h>
#include <inttypes.h>
#include "protocols.h"

/// Base size of the conversion buffer
#define BUFFER_BASE   4096
/// IANA enterprise number (forward fields)
#define IANA_EN_FWD   0
/// IANA enterprise number (reverse fields)
#define IANA_EN_REV   29305
/// IANA identificator of TCP flags
#define IANA_ID_FLAGS 6
/// IANA identificator of protocols
#define IANA_ID_PROTO 4

/// Conversion context
struct context {
    /// Data begin
    char *buffer_begin;
    /// The past-the-end element (a character that would follow the last character)
    char *buffer_end;
    /// Position of the next write operation
    char *write_begin;
    ///  Flag for realocation
    bool allow_real;
    ///  Other flags
    uint32_t flags;
    /// Information Element manager
    const fds_iemgr_t *mgr;
    /// Template snapshot
    const fds_tsnapshot_t *snap;
};

/**
 * \brief Conversion function callback
 * \param[in] buffer Conversion context
 * \param[in] field  Record field to convert to textual representation
 * \return #FDS_OK on success
 * \return #FDS_ERR_ARG in case of invalid data format
 * \return #FDS_ERR_NOMEM in case of memory allocation error
 */
typedef int (*converter_fn)(struct context *buffer, const struct fds_drec_field *field);

/**
 * \brief Get remaining size of the free space in the buffer
 * \param[in] buffer Conversion context
 * \return Number of free bytes
 */
static inline size_t
buffer_remain(const struct context *buffer)
{
    return buffer->buffer_end - buffer->write_begin;
}

/**
 * \brief Get total size of the buffer
 * \param[in] buffer Conversion context
 * \return Number of allocated bytes
 */
static inline size_t
buffer_alloc(const struct context *buffer)
{
    return buffer->buffer_end - buffer->buffer_begin;
}

/**
 * \brief Get size of the used space in the buffer
 * \param[in] buffer Conversion context
 * \return Number of used bytes
 */
static inline size_t
buffer_used(const struct context *buffer)
{
    return buffer->write_begin - buffer->buffer_begin;
}

/**
 * \brief Reserve memory of the conversion buffer
 *
 * Requests that the string capacity be adapted to a planned change in size to a length of up
 * to n characters.
 * \param[in] buffer Buffer
 * \param[in] n      Minimal size of the buffer
 * \return #FDS_OK on success
 * \return #FDS_ERR_NOMEM in case of a memory allocation error
 * \retunr #FDS_ERR_BUFFER in case the flag for reallocation is not set
 */
static inline int
buffer_reserve(struct context *buffer, size_t n)
{
    if (n <= buffer_alloc(buffer)) {
        // Nothing to do
        return FDS_OK;
    }
    if (buffer->allow_real == 0) {
        return FDS_ERR_BUFFER;
    }
    size_t used = buffer_used(buffer);

    // Prepare a new buffer and copy the content
    const size_t new_size = ((n / BUFFER_BASE) + 1) * BUFFER_BASE;
    char *new_buffer = (char*) realloc(buffer->buffer_begin, new_size * sizeof(char));
    if (new_buffer == NULL) {
        return FDS_ERR_NOMEM;
    }

    buffer->buffer_begin = new_buffer;
    buffer->buffer_end   = new_buffer + new_size;
    buffer->write_begin  = new_buffer + used;

    return FDS_OK;
}

/**
* \brief Append the conversion buffer
* \note
*   If the buffer length is not sufficient enough, it is automatically reallocated to fit
*   the string.
* \param[in] buffer Buffer
* \param[in] str    String to add
* \return #FDS_OK on success
* \return #FDS_ERR_NOMEM or #FDS_ERR_BUFFER in case of a memory allocation error
*/

static int
buffer_append(struct context *buffer, const char *str)
{
    const size_t len = strlen(str) + 1; // "\0"

    int ret_code = buffer_reserve(buffer, buffer_used(buffer) + len);
    if (ret_code != FDS_OK) {
        return ret_code;
    }

    memcpy(buffer->write_begin, str, len);
    buffer->write_begin += len - 1;
    return FDS_OK;
}

/**
 * \brief Convert an integer to JSON string
 * \param[in] buffer Buffer
 * \param[in] field  Field to convert
 * \return #FDS_OK on success
 * \return #FDS_ERR_ARG in case the field is not a valid field of this type
 * \return #FDS_ERR_NOMEM or #FDS_ERR_BUFFER in case of a memory allocation error
 */
static int
to_int(struct context *buffer, const struct fds_drec_field *field)
{
    // Print as signed integer
    int res = fds_int2str_be(field->data, field->size, buffer->write_begin, buffer_remain(buffer));
    if (res > 0) {
        buffer->write_begin += res;
        return FDS_OK;
    }

    if (res != FDS_ERR_BUFFER) {
        return FDS_ERR_ARG;
    }

    // Reallocate and try again
    int ret_code = buffer_reserve(buffer, buffer_used(buffer) + FDS_CONVERT_STRLEN_INT);
    if (ret_code != FDS_OK) {
        return ret_code;
    }

    return to_int(buffer, field);
}

/**
 * \brief Convert an unsigned integer to JSON string
 * \param[in] buffer Buffer
 * \param[in] field  Field to convert
 * \return #FDS_OK on success
 * \return #FDS_ERR_ARG in case the field is not a valid field of this type
 * \return #FDS_ERR_NOMEM or #FDS_ERR_BUFFER in case of a memory allocation error
 */
static int
to_uint(struct context *buffer, const struct fds_drec_field *field)
{
    // Print as unsigned integer
    int res = fds_uint2str_be(field->data, field->size, buffer->write_begin, buffer_remain(buffer));
    if (res > 0) {
        buffer->write_begin += res;
        return FDS_OK;
    }

    if (res != FDS_ERR_BUFFER) {
        return FDS_ERR_ARG;
    }

    // Reallocate and try again
    int ret_code = buffer_reserve(buffer, buffer_used(buffer) + FDS_CONVERT_STRLEN_INT);
    if (ret_code != FDS_OK) {
        return ret_code;
    }

    return to_uint(buffer, field);
}

/**
 * \brief Convert an octet array to JSON string
 *
 * \note
 *   Because the JSON doesn't directly support the octet array, the result string is wrapped in
 *   double quotes.
 * \param[in] buffer Buffer
 * \param[in] field  Field to convert
 * \return #FDS_OK on success
 * \return #FDS_ERR_ARG in case the field is not a valid field of this type
 * \return #FDS_ERR_NOMEM or #FDS_ERR_BUFFER in case of a memory allocation error
 */
static int
to_octet(struct context *buffer, const struct fds_drec_field *field)
{
    if (field->size == 0) {
        // Empty field cannot be converted -> null
        return FDS_ERR_ARG;
    }

    if ((buffer->flags & FDS_CD2J_OCTETS_NOINT) == 0 && field->size <= 8) {
        // Print as unsigned integer
        return to_uint(buffer, field);
    }

    // Print as hexadecimal number
    const size_t mem_req = (2 * field->size) + 5U; // "0x" + 2 chars per byte + 2x "\"" + "\0"

    int ret_code = buffer_reserve(buffer, buffer_used(buffer) + mem_req);
    if (ret_code != FDS_OK) {
        return ret_code;
    }

    buffer->write_begin[0] = '"';
    buffer->write_begin[1] = '0';
    buffer->write_begin[2] = 'x';
    buffer->write_begin += 3;
    int res = fds_octet_array2str(field->data, field->size, buffer->write_begin, buffer_remain(buffer));
    if (res >= 0) {
        buffer->write_begin += res;
        *(buffer->write_begin++) = '"';
        return FDS_OK;
    }

    // Error
    return FDS_ERR_ARG;
}

/**
 * \brief Convert a float to JSON string
 *
 * \note
 *   If the value represent infinite or NaN, instead of number a corresponding string
 *   is stored.
 * \param[in] buffer Buffer
 * \param[in] field  Field to convert
 * \return #FDS_OK on success
 * \return #FDS_ERR_ARG if failed to convert a float number
 * \return #FDS_ERR_NOMEM or #FDS_ERR_BUFFER in case of a memory allocation error
 */
static int
to_float(struct context *buffer, const struct fds_drec_field *field)
{
    // We can not use default function because of "NAN" and "infinity" values
    double value;
    if (fds_get_float_be(field->data, field->size, &value) != FDS_OK) {
        return FDS_ERR_ARG;
    }

    if (isfinite(value)) {
        // Normal value
        const char *fmt = (field->size == sizeof(float))
            ? ("%." FDS_CONVERT_STRX(FLT_DIG) "g")  // float precision
            : ("%." FDS_CONVERT_STRX(DBL_DIG) "g"); // double precision
        int str_real_len = snprintf(buffer->write_begin, buffer_remain(buffer), fmt, value);
        if (str_real_len < 0) {
            return FDS_ERR_ARG;
        }

        if ((size_t) str_real_len >= buffer_remain(buffer)) {
            // Reallocate the buffer and try again
            int ret_code = buffer_reserve(buffer, 2 * buffer_alloc(buffer)); // Just double it
            if (ret_code != FDS_OK) {
                return ret_code;
            }
            return to_float(buffer, field);
        }

        buffer->write_begin += str_real_len;
        return FDS_OK;
    }

    // +/-Nan or +/-infinite
    const char *str;
    // Size 8 (double)
    if (isinf(value) && value >= 0) {
        str = "\"Infinity\"";
    } else if (isinf(value) && value < 0) {
        str = "\"-Infinity\"";
    } else if (isnan(value)) {
        str = "\"NaN\"";
    } else {
        str = "null";
    }

    // size without '\0'
    size_t size = strlen(str);

    // +1 because strcpy copy '\0' too, so there need to be reserved more, then 'size'
    int ret_code = buffer_reserve(buffer, buffer_used(buffer) + size + 1);
    if (ret_code != FDS_OK) {
        return ret_code;
    }

    // copy with '\0'
    strcpy(buffer->write_begin, str);
    buffer->write_begin += size;
    return FDS_OK;
}

/**
 * \brief Convert a boolean to JSON string
 *
 * \note If the stored boolean value is invalid, an exception is thrown!
 * \param[in] buffer Buffer
 * \param[in] field Field to convert
 * \return #FDS_OK on success
 * \return #FDS_ERR_ARG if failed to convert a boolean to string
 * \return #FDS_ERR_NOMEM or #FDS_ERR_BUFFER in case of a memory allocation error
 */
static int
to_bool(struct context *buffer, const struct fds_drec_field *field)
{
    if (field->size != 1) {
        return FDS_ERR_ARG;
    }

    int res = fds_bool2str(field->data, buffer->write_begin, buffer_remain(buffer));
    if (res > 0) {
        buffer->write_begin += res;
        return FDS_OK;
    }

    if (res != FDS_ERR_BUFFER) {
        return FDS_ERR_ARG;
    }

    // Reallocate and try again
    int ret_code = buffer_reserve(buffer, buffer_used(buffer) + FDS_CONVERT_STRLEN_FALSE); // false is longer
    if (ret_code != FDS_OK) {
        return ret_code;
    }

    return to_bool(buffer, field);
}


/**
 * \brief Convert a datetime to JSON string
 *
 * Based on the configuration, the output string is formatted or represent UNIX timestamp
 * (in milliseconds). Formatted string is based on ISO 8601 and use only millisecond precision
 * because JSON parsers typically doesn't support anything else.
 * \param[in] buffer Buffer
 * \param[in] field  Field to convert
 * \return #FDS_OK on success
 * \return #FDS_ERR_ARG if failed to convert a timestamp to string
 * \return #FDS_ERR_NOMEM or #FDS_ERR_BUFFER in case of a memory allocation error
 */
static int
to_datetime(struct context *buffer, const struct fds_drec_field *field)
{
    const enum fds_iemgr_element_type type = field->info->def->data_type;

    if ((buffer->flags & FDS_CD2J_TS_FORMAT_MSEC) != 0) {
        // Convert to formatted string
        enum fds_convert_time_fmt fmt = FDS_CONVERT_TF_MSEC_UTC; // Only supported by JSON parser

        int ret_code = buffer_reserve(buffer, buffer_used(buffer) + FDS_CONVERT_STRLEN_DATE + 2); // 2x '\"'
        if (ret_code != FDS_OK) {
            return ret_code;
        }

        *(buffer->write_begin++) = '"';
        int res = fds_datetime2str_be(field->data, field->size, type, buffer->write_begin,
            buffer_remain(buffer), fmt);
        if (res > 0) {
            // Success
            buffer->write_begin += res;
            *(buffer->write_begin++) = '"';
            return FDS_OK;
        }

        return FDS_ERR_ARG;
    }

    // Convert to UNIX timestamp (in milliseconds)
    uint64_t time;
    if (fds_get_datetime_lp_be(field->data, field->size, type, &time) != FDS_OK) {
        return FDS_ERR_ARG;
    }

    time = htobe64(time); // Convert to network byte order and use fast libfds converter
    int res = fds_uint2str_be(&time, sizeof(time), buffer->write_begin, buffer_remain(buffer));
    if (res > 0) {
        buffer->write_begin += res;
        return FDS_OK;
    }

    if (res != FDS_ERR_BUFFER) {
        return FDS_ERR_ARG;
    }

    int ret_code = buffer_reserve(buffer, buffer_used(buffer) + FDS_CONVERT_STRLEN_INT);
    if (ret_code != FDS_OK) {
        return ret_code;
    }

    return to_datetime(buffer, field);
}


/**
 * \brief Convert a MAC address to JSON string
 *
 * \note
 *   Because the JSON doesn't directly support the MAC address, the result string is wrapped in
 *   double quotes.
 * \param[in] buffer Buffer
 * \param[in] field  Field to convert
 * \return #FDS_OK on success
 * \return #FDS_ERR_ARG if failed to convert a MAC address to string
 * \return #FDS_ERR_NOMEM or #FDS_ERR_BUFFER in case of a memory allocation error
 */
static int
to_mac(struct context *buffer, const struct fds_drec_field *field)
{
    int ret_code = buffer_reserve(buffer, buffer_used(buffer) + FDS_CONVERT_STRLEN_MAC + 2); // MAC address + 2x '\"'
    if (ret_code != FDS_OK) {
        return ret_code;
    }

    *(buffer->write_begin++) = '"';
    int res = fds_mac2str(field->data, field->size, buffer->write_begin, buffer_remain(buffer));
    if (res > 0) {
        buffer->write_begin += res;
        *(buffer->write_begin++) = '"';
        return FDS_OK;
    }

    return FDS_ERR_ARG;
}

/**
 * \brief Convert an IPv4/IPv6 address to JSON string
 *
 * \note
 *   Because the JSON doesn't directly support IP addresses, the result string is wrapped in double
 *   quotes.
 * \param[in] field  Field to convert
 * \param[in] buffer Buffer
 * \return #FDS_OK on success
 * \return #FDS_ERR_ARG if failed to convert an IP address to string
 * \return #FDS_ERR_NOMEM or #FDS_ERR_BUFFER in case of a memory allocation error
 */
static int
to_ip(struct context *buffer, const struct fds_drec_field *field)
{
    // Make sure that we have enough memory
    int ret_code = buffer_reserve(buffer, buffer_used(buffer) + FDS_CONVERT_STRLEN_IP + 2); // IPv4/IPv6 address + 2x '\"'
    if (ret_code != FDS_OK){
        return ret_code;
    }

    *(buffer->write_begin++) = '"';
    int res = fds_ip2str(field->data, field->size, buffer->write_begin, buffer_remain(buffer));
    if (res > 0) {
        buffer->write_begin += res;
        *(buffer->write_begin++) = '"';
        return FDS_OK;
    }

    return FDS_ERR_ARG;
}

/**
 * \brief Is a UTF-8 character valid
 * \param[in] str Pointer to the character beginning
 * \param[in] len Maximum length of the character (in bytes)
 * \note Parameter \p len is used to avoid access outside of the array's bounds.
 * \warning Value of the parameter \p len MUST be at least 1.
 * \return If the character is NOT valid, the function will return 0.
 *   Otherwise (only valid characters) returns length of the character i.e.
 *   number 1-4 (in bytes).
 */
static inline int
utf8char_is_valid(const uint8_t *str, size_t len)
{
    if ((str[0] & 0x80) == 0) {                // 0xxx xxxx
        // Do nothing...
        return 1;
    }

    if ((str[0] & 0xE0) == 0xC0 && len >= 2) { // 110x xxxx + 1 more byte
        // Check the second byte (must be 10xx xxxx)
        return ((str[1] & 0xC0) == 0x80) ? 2 : 0;
    }

    if ((str[0] & 0xF0) == 0xE0 && len >= 3) { // 1110 xxxx + 2 more bytes
        // Check 2 tailing bytes (each must be 10xx xxxx)
        uint16_t tail = *((const uint16_t *) &str[1]);
        return ((tail & 0xC0C0) == 0x8080) ? 3 : 0;
    }

    if ((str[0] & 0xF8) == 0xF0 && len >= 4) { // 1111 0xxx + 3 more bytes
        // Check 3 tailing bytes (each must be 10xx xxxx)
        uint32_t tail = *((const uint32_t *) &str[0]);
        // Change the first byte for easier comparision
        *(uint8_t *) &tail = 0x80; // Little/big endian compatible solution
        return ((tail & 0xC0C0C0C0) == 0x80808080) ? 4 : 0;
    }

    // Invalid character
    return 0;
}

/**
 * \brief Is it a '\' or '"' character
 * \param[in]  str  Pointer to the character beginning
 * \param[in]  len  Maximum length of the character (in bytes)
 * \param[out] repl Replacement character (can be NULL)
 * \return True or false.
*/
static inline bool
utf8char_is_not_esc(const uint8_t *str, size_t len, uint8_t *repl)
{
    (void) len;

    uint8_t new_char;
    const char old_char = (char) *str;

    switch (old_char) {
    case '\\':
        new_char = '\\';
        break;
    case '\"':
        new_char = '\"';
        break;
    default:
        return false;
    }

    if (repl != NULL) {
        *repl = new_char;
    }

    return true;
}

/**
 * \brief Is it a UTF-8 control character
 * \param[in] str Pointer to the character beginning
 * \param[in] len Maximum length of the character (in bytes)
 * \note Parameter \p len is used to avoid access outside of the array's bounds.
 * \return True or false.
 */
static inline bool
utf8char_is_control(const uint8_t *str, size_t len)
{
    (void) len;

    // Check C0 control characters
    if (str[0] <= 0x1F || str[0] == 0x7F) {
        return true;
    }

    // Check C1 control characters
    if (str[0] >= 0x80 && str[0] <= 0x9F) {
        return true;
    }
    return false;
}

/**
 * \brief Is a UTF-8 character escapable
 * \param[in]  str  Pointer to the character beginning
 * \param[in]  len  Maximum length of the character (in bytes)
 * \param[out] repl Replacement character (can be NULL). The variable is filled
 *   only when the input character is escapable. The size of the character is
 *   always only one byte.
 * \note Parameter \p len is used to avoid access outside of the array's bounds.
 * \return True or false.
 */
static inline bool
utf8char_is_escapable(const uint8_t *str, size_t len, uint8_t *repl)
{
    (void) len;
    if ((str[0] & 0x80) != 0) {
        // Only 1 byte characters can be escapable (for now)
        return false;
    }

    uint8_t new_char;
    const char old_char = (char) *str;

    switch (old_char) {
        case '\n': new_char = 'n'; break;
        case '\r': new_char = 'r'; break;
        case '\t': new_char = 't'; break;
        case '\b': new_char = 'b'; break;
        case '\f': new_char = 'f'; break;
        default: return false;
    }

    if (repl != NULL) {
        *repl = new_char;
    }

    return true;
}

/**
 * \brief Convert IPFIX string to JSON string
 *
 * Quote and backslash are always escaped and white space (and control) characters are converted
 * based on active configuration.
 * \param[in] field  Field to convert
 * \param[in] buffer Buffer
 * \return #FDS_OK on success
 * \return #FDS_ERR_ARG if failed to convert an IP address to string
 * \return #FDS_ERR_NOMEM or #FDS_ERR_BUFFER in case of a memory allocation error
 */
static int
to_string(struct context *buffer, const struct fds_drec_field *field)
{
    /* Make sure that we have enough memory for the worst possible case (escaping everything)
     * This case contains only non-printable characters that will be replaced with string
     * "\uXXXX" (6 characters) each.
     */
    const size_t max_size = (6 * field->size) + 4U; // '\uXXXX' + 2x "\"" + 1x '\0'
    int ret_code = buffer_reserve(buffer, buffer_used(buffer) + max_size);
    if (ret_code != FDS_OK) {
        return ret_code;
    }

    size_t size = field->size;
    unsigned int step;
    size_t pos_copy = 0; // Start of "copy" region
    uint8_t subst; // Replacement character for escapable characters

    const uint8_t *in_buffer = (const uint8_t *) (field->data);
    uint8_t *out_buffer = (uint8_t *) buffer->write_begin;
    uint32_t pos_out = 0;

    // Beginning of the string
    out_buffer[pos_out] = '"';
    pos_out += 1;

    for (size_t pos_in = 0; pos_in < size; pos_in += step) {

        const uint8_t *char_ptr = in_buffer + pos_in;
        const size_t char_max = field->size - pos_in; // Maximum character length

        int      is_valid =  utf8char_is_valid(char_ptr, char_max);
        bool is_escapable =  utf8char_is_escapable(char_ptr, char_max, &subst);
        bool   is_control =  utf8char_is_control(char_ptr, char_max);
        bool   is_not_esc =  utf8char_is_not_esc(char_ptr, char_max, &subst);

        // Size of the current character
        step = (is_valid > 0) ? (unsigned int) is_valid : 1;

        if (is_valid && !is_escapable && !is_control && !is_not_esc) {
            continue;
        }

        // -- Interpretation of the character must be changed --
        // Copy unchanged characters
        const size_t copy_len = pos_in - pos_copy;
        memcpy(&out_buffer[pos_out], &in_buffer[pos_copy], copy_len);
        pos_out += copy_len;
        pos_copy = pos_in + 1; // Next time start from the next character

        /*
         * Based on RFC 4627 (Section: 2.5. Strings):
         * Control characters '\' and '"' must be escaped using '\\' and '\"'.
         */
        if (is_not_esc) {
            const size_t subst_len = 2U;
            out_buffer[pos_out] = '\\';
            out_buffer[pos_out + 1] = subst;
            pos_out += subst_len;
            continue;
        }

        // Escape characte only if flag FDS_CD2J_NON_PRINTABLE is set
        if ((buffer->flags & FDS_CD2J_NON_PRINTABLE) != 0) {
            continue;
        }

        // Is it an escapable character?
        if (is_escapable) {
            const size_t subst_len = 2U;
            out_buffer[pos_out] = '\\';
            out_buffer[pos_out + 1] = subst;
            pos_out += subst_len;
            continue;
        }

        /*
         * Based on RFC 4627 (Section: 2.5. Strings):
         * Control characters (i.e. 0x00 - 0x1F) must be escaped
         * using "\uXXXX" where "XXXX" is a hexa value.
         */
        // Is it a control character?
        if (is_control) {
            const size_t subst_len = 6U;
            uint8_t hex;

            out_buffer[pos_out] = '\\';
            out_buffer[pos_out + 1] = 'u';
            out_buffer[pos_out + 2] = '0';
            out_buffer[pos_out + 3] = '0';

            hex = ((*char_ptr) & 0xF0) >> 4;
            out_buffer[pos_out + 4] = (hex < 10) ? ('0' + hex) : ('A' - 10 + hex);
            hex = (*char_ptr) & 0x0F;
            out_buffer[pos_out + 5] = (hex < 10) ? ('0' + hex) : ('A' - 10 + hex);
            pos_out += subst_len;

            continue;
        }

        // Invalid character -> replace with "REPLACEMENT CHARACTER"
        const size_t subst_len = 3U;
        out_buffer[pos_out] = 0xEF;   // Character U+FFFD in UTF8 encoding
        out_buffer[pos_out + 1] = 0xBF;
        out_buffer[pos_out + 2] = 0xBD;
        pos_out += subst_len;

    }
    const size_t copy_len = size - pos_copy;
    memcpy(&out_buffer[pos_out], &in_buffer[pos_copy], copy_len);
    pos_out += copy_len;
    out_buffer[pos_out++] = '"';

    // End of the string
    buffer->write_begin += pos_out;
    return FDS_OK;
}

/**
 * \brief Convert TCP flags to JSON string
 *
 * \note The result string is wrapped in double quotes.
 * \param[in] field  Field to convert
 * \param[in] buffer Buffer
 * \return #FDS_OK on success
 * \return #FDS_ERR_ARG if failed to convert TCP flags to string
 * \return #FDS_ERR_NOMEM or #FDS_ERR_BUFFER in case of a memory allocation error
 */
static int
to_flags(struct context *buffer, const struct fds_drec_field *field)
{
    if (field->size != 1 && field->size != 2) {
        return FDS_ERR_ARG;
    }

    uint8_t flags;
    if (field->size == 1) {
        flags = *field->data;
    } else {
        flags = ntohs(*(uint16_t*)(field->data));
    }

    const size_t size = 8; // 2x '"' + 6x flags
    int ret_code = buffer_reserve(buffer, buffer_used(buffer) + size + 1); // '\0'
    if (ret_code != FDS_OK) {
        return ret_code;
    }

    char *buff = buffer->write_begin;
    buff[0] = '"';
    buff[1] = (flags & 0x20) ? 'U' : '.';
    buff[2] = (flags & 0x10) ? 'A' : '.';
    buff[3] = (flags & 0x08) ? 'P' : '.';
    buff[4] = (flags & 0x04) ? 'R' : '.';
    buff[5] = (flags & 0x02) ? 'S' : '.';
    buff[6] = (flags & 0x01) ? 'F' : '.';
    buff[7] = '"';
    buff[8] = '\0';

    buffer->write_begin += size;
    return FDS_OK;
}

/**
 * \brief Convert a protocol to JSON string
 *
 * \note The result string is wrapped in double quotes.
 * \param[in] field  Field to convert
 * \param[in] buffer Buffer
 * \return #FDS_OK on success
 * \return #FDS_ERR_ARG if failed to convert a protocol to string
 * \return #FDS_ERR_NOMEM or #FDS_ERR_BUFFER in case of a memory allocation error
 */
static int
to_proto(struct context *buffer, const struct fds_drec_field *field)
{
    if (field->size != 1) {
        return FDS_ERR_ARG;
    }

    const char *proto_str = protocols[*field->data];
    const size_t proto_len = strlen(proto_str);
    int ret_code = buffer_reserve(buffer, buffer_used(buffer) + proto_len + 3); // 2x '"' + '\0'
    if (ret_code != FDS_OK){
        return ret_code;
    }

    *(buffer->write_begin++) = '"';
    memcpy(buffer->write_begin, proto_str, proto_len);
    buffer->write_begin += proto_len;
    *(buffer->write_begin++) = '"';
    return FDS_OK;
}
/**
 * \breaf Auxiliary function for converting record fields with multiple occurrence
 *
 * The values are stored into a simple JSON array identified by the field ID.
 * \param[in] rec       IPFIX Data Record with the fields
 * \param[in] buffer    Buffer
 * \param[in] fn        Convert function applied on the values
 * \param[in] en        Enterprise Number of the field
 * \param[in] id        Information Element ID of the field
 * \param[in] iter_flag Data Record iterator flags
 *
 * \return #FDS_OK on success
 * \return #FDS_ERR_NOMEM or #FDS_ERR_BUFFER in case of a memory allocation error
 */
static int
multi_fields(const struct fds_drec *rec, struct context *buffer,
    converter_fn fn, uint32_t en, uint16_t id, uint16_t iter_flag)
{
    // Initialization of an iterator
    struct fds_drec_iter iter_mul_f;
    fds_drec_iter_init(&iter_mul_f, (struct fds_drec *) rec, iter_flag);

    // Multi-fields must be stored as "enXX:idYY":[value, value...]
    int ret_code;
    // Append "["
    ret_code = buffer_append(buffer, "[");
    if (ret_code != FDS_OK) {
        return ret_code;
    }

    // Looking for multi fields with the given ID
    while (fds_drec_iter_next(&iter_mul_f) != FDS_EOC) {
        const struct fds_tfield *def = iter_mul_f.field.info;
        const size_t writer_offset = buffer_used(buffer);

        // Check the ID and enterprise number
        if (def->id != id || def->en != en) {
            continue;
        }

        ret_code = fn(buffer, &iter_mul_f.field);

        switch (ret_code) {
        // Recover from a conversion error
        case FDS_ERR_ARG:
            // Conversion error: return to the previous position (note: realloc() might happened)
            buffer->write_begin = buffer->buffer_begin + writer_offset;
            ret_code = buffer_append(buffer, "null");
            if (ret_code != FDS_OK) {
                return ret_code;
            }
            // fallthrough
        case FDS_OK:
            break;
        default:
           // Other errors -> completely out
           return ret_code;
        }

        // If it is the last field, then go out from this loop
        if (def->flags & FDS_TFIELD_LAST_IE) {
            break;
        }

        // Otherwise add "," and continue
        ret_code = buffer_append(buffer, ",");
        if (ret_code != FDS_OK) {
            return ret_code;
        }
        continue;
    }

    // add "]" in the end if there are no more fields with same ID or EN
    ret_code = buffer_append(buffer, "]" );
    if (ret_code != FDS_OK) {
        return ret_code;
    }

    return FDS_OK;
}

static int
to_blist (struct context *buffer, const struct fds_drec_field *field);
static int
add_field_name(struct context *buffer, const struct fds_drec_field *field);
static int
to_stlist(struct context *buffer, const struct fds_drec_field *field);
static int
to_stMulList(struct context *buffer, const struct fds_drec_field *field);

/**
 * \brief Find a conversion function for an IPFIX field
 * \param[in] field An IPFIX field to convert
 * \return Conversion function
 */
static converter_fn
get_converter(const struct fds_drec_field *field)
{
    // Conversion table, based on types defined by enum fds_iemgr_element_type
    static const converter_fn table[] = {
        &to_octet,    // FDS_ET_OCTET_ARRAY
        &to_uint,     // FDS_ET_UNSIGNED_8
        &to_uint,     // FDS_ET_UNSIGNED_16
        &to_uint,     // FDS_ET_UNSIGNED_32
        &to_uint,     // FDS_ET_UNSIGNED_64
        &to_int,      // FDS_ET_SIGNED_8
        &to_int,      // FDS_ET_SIGNED_16
        &to_int,      // FDS_ET_SIGNED_32
        &to_int,      // FDS_ET_SIGNED_64
        &to_float,    // FDS_ET_FLOAT_32
        &to_float,    // FDS_ET_FLOAT_64
        &to_bool,     // FDS_ET_BOOLEAN
        &to_mac,      // FDS_ET_MAC_ADDRESS
        &to_string,   // FDS_ET_STRING
        &to_datetime, // FDS_ET_DATE_TIME_SECONDS
        &to_datetime, // FDS_ET_DATE_TIME_MILLISECONDS
        &to_datetime, // FDS_ET_DATE_TIME_MICROSECONDS
        &to_datetime, // FDS_ET_DATE_TIME_NANOSECONDS
        &to_ip,       // FDS_ET_IPV4_ADDRESS
        &to_ip,       // FDS_ET_IPV6_ADDRESS
        &to_blist,    // FDS_ET_BASIC_LIST
        &to_stlist,   // FDS_ET_SUB_TEMPLATE_LIST
        &to_stMulList // FDS_ET_SUB_TEMPLATE_MULTILIST
        // Other types are not supported yet
    };

    const size_t table_size = sizeof(table) / sizeof(table[0]);
    const enum fds_iemgr_element_type type = (field->info->def != NULL)
        ? (field->info->def->data_type) : FDS_ET_OCTET_ARRAY;

    if (type >= table_size) {
        return &to_octet;
    } else {
        return table[type];
    }
}

/**
 * \breaf Function for iterating through Information Elements
 * \param[in] rec    IPFIX Data Record to convert
 * \param[in] buffer Buffer
 *
 * \return #FDS_OK on success
 * \return #FDS_ERR_NOMEM or #FDS_ERR_BUFFER in case of a memory allocation error
 */
static int
iter_loop(const struct fds_drec *rec, struct context *buffer)
{
    converter_fn fn;
    unsigned int added = 0;
    int ret_code;

    // Initialize iterator flags
    uint16_t iter_flag = 0;
    iter_flag |= (buffer->flags & FDS_CD2J_IGNORE_UNKNOWN) ? FDS_DREC_UNKNOWN_SKIP : 0;
    iter_flag |= (buffer->flags & FDS_CD2J_BIFLOW_REVERSE) ? FDS_DREC_BIFLOW_REV   : 0;
    iter_flag |= (buffer->flags & FDS_CD2J_REVERSE_SKIP)   ? FDS_DREC_REVERSE_SKIP : 0;

    struct fds_drec_iter iter;
    fds_drec_iter_init(&iter, (struct fds_drec *) rec, iter_flag);

    while (fds_drec_iter_next(&iter) != FDS_EOC) {
        // If flag of multi fields is set,
        // then this field will be skiped and processed when the last occurrence is found
        const fds_template_flag_t field_flags = iter.field.info->flags;
        if ((field_flags & FDS_TFIELD_MULTI_IE) != 0 && (field_flags & FDS_TFIELD_LAST_IE) == 0){
            continue;
        }

        // Separate fields
        if (added != 0) {
            // Add comma
            ret_code = buffer_append(buffer, ",");
            if (ret_code != FDS_OK) {
                return ret_code;
            }
        }

        // Add field name "<pen>:<field_name>"
        ret_code = add_field_name(buffer, &iter.field);
        if (ret_code != FDS_OK) {
            return ret_code;
        }

        // Find a converter
        const struct fds_tfield *def = iter.field.info;
        if ((buffer->flags & FDS_CD2J_FORMAT_TCPFLAGS) && def->id == IANA_ID_FLAGS
                && (def->en == IANA_EN_FWD || def->en == IANA_EN_REV)) {
            // Convert to formatted flags
            fn = &to_flags;
        } else if ((buffer->flags & FDS_CD2J_FORMAT_PROTO) && def->id == IANA_ID_PROTO
                && (def->en == IANA_EN_FWD || def->en == IANA_EN_REV)) {
            // Convert to formatted protocol type
            fn = &to_proto;
        } else {
            // Convert to field based on the internal type
            fn = get_converter(&iter.field);
        }

        // Convert the field
        const size_t writer_offset = buffer_used(buffer);
        if ((field_flags & FDS_TFIELD_MULTI_IE) != 0 && (field_flags & FDS_TFIELD_LAST_IE) != 0) {
            // Conversion of the field with multiple occurrences
           ret_code = multi_fields(rec, buffer, fn, def->en, def->id, iter_flag);
        } else {
           ret_code = fn(buffer, &iter.field);
        }

        switch (ret_code) {
        // Recover from a conversion error
        case FDS_ERR_ARG:
            // Conversion error: return to the previous position (note: realloc() might happened)
            buffer->write_begin = buffer->buffer_begin + writer_offset;
            ret_code = buffer_append(buffer, "null");
            if (ret_code != FDS_OK) {
                return ret_code;
            }
            __attribute__ ((fallthrough)); // only for purpose to avoid warning "this statement may fall through"

        case FDS_OK:
            added++;
            continue;
        default:
            // Other errors -> completely out
            return ret_code;
        }
    }

    return FDS_OK;
}

/**
 * \breaf Function for adding semantic for structured datatype
 * \param[in] buffer Buffer
 * \param[in] semantic Semantic value
 *
 * \return #FDS_OK on success
 * \return #FDS_ERR_NOMEM or #FDS_ERR_BUFFER in case of a memory allocation error
 */
static int
add_sematic(struct context *buffer, int semantic) {
    int ret_code;

    switch (semantic) {
    case FDS_IPFIX_LIST_NONE_OF:
        ret_code = buffer_append(buffer, "noneOf");
        break;
    case FDS_IPFIX_LIST_EXACTLY_ONE_OF:
        ret_code = buffer_append(buffer, "exactlyOneOf");
        break;
    case FDS_IPFIX_LIST_ONE_OR_MORE_OF:
        ret_code = buffer_append(buffer, "oneOrMoreOf");
        break;
    case FDS_IPFIX_LIST_ALL_OF:
        ret_code = buffer_append(buffer, "allOf");
        break;
    case FDS_IPFIX_LIST_ORDERED:
        ret_code = buffer_append(buffer, "ordered");
        break;
    default:
        ret_code = buffer_append(buffer, "undefined");
        break;
    }
    return ret_code;
}

/**
 * \brief Process basicList data type
 *
 * \param[in] field  An IPFIX field to convert
 * \param[in] buffer Buffer
 * \return #FDS_OK on success
 * \return #FDS_ERR_ARG if the basicList is malformed
 * \return #FDS_ERR_NOMEM or #FDS_ERR_BUFFER in case of a memory allocation error
 */
int
to_blist(struct context *buffer, const struct fds_drec_field *field)
{
    int ret_code;
    int added = 0;
    struct fds_blist_iter blist_iter;

    ret_code = buffer_append(buffer,"{\"@type\":\"basicList\",\"data\":[");
    if (ret_code != FDS_OK) {
        return ret_code;
    }

    fds_blist_iter_init(&blist_iter, (struct fds_drec_field *) field, buffer->mgr);
    converter_fn fn = NULL;

    // Add values from the list
    int rc_iter;
    while ((rc_iter = fds_blist_iter_next(&blist_iter)) == FDS_OK) {
        if (added > 0) {
            // Add comma
            ret_code = buffer_append(buffer,",");
            if (ret_code != FDS_OK) {
                return ret_code;
            }
        }

        if (fn == NULL) {
            // The first record -> determine the converter
            fn = get_converter(&blist_iter.field);
        }

        // Convert the field
        const size_t writer_offset = buffer_used(buffer);
        ret_code = fn(buffer, &blist_iter.field);

        switch (ret_code) {
        case FDS_ERR_ARG:
            // Conversion error: return to the previous position (note: realloc() might happened)
            buffer->write_begin = buffer->buffer_begin + writer_offset;
            ret_code = buffer_append(buffer, "null");
            if (ret_code != FDS_OK) {
                return ret_code;
            }
            __attribute__ ((fallthrough)); // only for purpose to avoid warning "this statement may fall through"
         case FDS_OK:
            added++;
            continue;
        default:
            // Other errors ->completely out
            return ret_code;
        }
    }

    if (rc_iter != FDS_EOC) {
        // Iterator failed!
        return FDS_ERR_ARG;
    }

    // Add semantic and Field ID
    ret_code = buffer_append(buffer, "],\"semantic\":\"");
    if (ret_code != FDS_OK) {
        return ret_code;
    }

    ret_code = add_sematic(buffer, blist_iter.semantic);
    if (ret_code != FDS_OK) {
        return ret_code;
    }

    ret_code = buffer_append(buffer,"\",\"fieldID\":");
    if (ret_code != FDS_OK) {
        return ret_code;
    }

    ret_code = add_field_name(buffer, &blist_iter.field);
    if (ret_code != FDS_OK) {
        return ret_code;
    }

    // We have to "remove" the additional colon after the identifier
    --buffer->write_begin;

    ret_code = buffer_append(buffer,"}");
    if (ret_code != FDS_OK){
        return ret_code;
    }

    return FDS_OK;
}

/**
 * \brief Process subTemplateList datatype
 *
 * \param[in] field  An IPFIX field to convert
 * \param[in] buffer Buffer
 * \return #FDS_OK on success
 * \return #FDS_ERR_ARG if the subTemplateList is malformed
 * \return #FDS_ERR_NOMEM or #FDS_ERR_BUFFER in case of a memory allocation error
 */
int
to_stlist(struct context *buffer, const struct fds_drec_field *field)
{
    int ret_code;
    int added = 0;
    struct fds_stlist_iter stlist_iter;

    ret_code = buffer_append(buffer,"{\"@type\":\"subTemplateList\",\"semantic\":\"");
    if (ret_code != FDS_OK) {
        return ret_code;
    }

    fds_stlist_iter_init(&stlist_iter, (struct fds_drec_field *) field, buffer->snap, 0);

    // Add semantic
    ret_code = add_sematic(buffer, stlist_iter.semantic);
    if (ret_code != FDS_OK) {
        return ret_code;
    }

    ret_code = buffer_append(buffer,"\",\"data\":[");
    if (ret_code != FDS_OK) {
        return ret_code;
    }

    // Add records from the list
    int rc_iter;
    while ((rc_iter = fds_stlist_iter_next(&stlist_iter)) == FDS_OK) {
        if (added > 0) {
            // Add comma
            ret_code = buffer_append(buffer,",");
            if (ret_code != FDS_OK) {
                return ret_code;
            }
        }

        // Add "{" in the beginning of each structure
        ret_code = buffer_append(buffer,"{");
        if (ret_code != FDS_OK) {
            return ret_code;
        }

        ret_code = iter_loop(&stlist_iter.rec, buffer);
        if (ret_code != FDS_OK) {
            return ret_code;
        }

        // Add "{" in the beginning of each structure
        ret_code = buffer_append(buffer,"}");
        if (ret_code != FDS_OK) {
            return ret_code;
        }

        added++;
    }

    if (rc_iter != FDS_EOC) {
        // Iterator failed!
        return FDS_ERR_ARG;
    }

    ret_code = buffer_append(buffer,"]}");
    if (ret_code != FDS_OK) {
        return ret_code;
    }

    return FDS_OK;
}

/**
 * \brief Process subTemplteMultiList datatype
 *
 * \param[in] field An IPFIX field to convert
 * \param[in] buffer Buffer
 * \return #FDS_OK on success
 * \return #FDS_ERR_ARG if the basicList is malformed
 * \return #FDS_ERR_NOMEM or #FDS_ERR_BUFFER in case of a memory allocation error
 */
int
to_stMulList(struct context *buffer, const struct fds_drec_field *field)
{
    int ret_code;
    struct fds_stmlist_iter stMulList_iter;

    ret_code = buffer_append(buffer,"{\"@type\":\"subTemplateMultiList\",\"semantic\":\"");
    if (ret_code != FDS_OK) {
        return ret_code;
    }

    fds_stmlist_iter_init(&stMulList_iter, (struct fds_drec_field *) field, buffer->snap, 0);

    // Add semantic
    ret_code = add_sematic(buffer, stMulList_iter.semantic);
    if (ret_code != FDS_OK) {
        return ret_code;
    }

    ret_code = buffer_append(buffer,"\",\"data\":[");
    if (ret_code != FDS_OK) {
        return ret_code;
    }

    // Loop through blocks in the list
    int added = 0;
    int rc_block;
    int rc_rec = FDS_EOC; // Default value (the block might be empty)

    while ((rc_block = fds_stmlist_iter_next_block(&stMulList_iter)) == FDS_OK) {
        // Separate fields
        if (added > 0) {
            // Add comma
            ret_code = buffer_append(buffer,",");
            if (ret_code != FDS_OK) {
                return ret_code;
            }
        }
        // Add opening bracket for block
        ret_code = buffer_append(buffer,"[");
        if (ret_code != FDS_OK) {
            return ret_code;
        }

        // Loop through individual records in the current block
        int added_in_block = 0;
        while ((rc_rec = fds_stmlist_iter_next_rec(&stMulList_iter)) == FDS_OK) {
            // Separate fields
            if (added_in_block > 0) {
                // Add comma
                ret_code = buffer_append(buffer,",");
                if (ret_code != FDS_OK) {
                    return ret_code;
                }
            }
            // Add opening bracket for the record
            ret_code = buffer_append(buffer,"{");
            if (ret_code != FDS_OK) {
                return ret_code;
            }

            ret_code = iter_loop(&stMulList_iter.rec, buffer);
            if (ret_code != FDS_OK) {
                return ret_code;
            }

            // Add closing bracket for the record
            ret_code = buffer_append(buffer,"}");
            if (ret_code != FDS_OK) {
                return ret_code;
            }
            added_in_block++;
        }

        if (rc_rec != FDS_EOC) {
            // Something went wrong!
            break;
        }

        // Add closing bracket for block
        ret_code = buffer_append(buffer,"]");
        if (ret_code != FDS_OK) {
            return ret_code;
        }
        added++;
    }

    if (rc_block != FDS_EOC || rc_rec != FDS_EOC) {
        // Iterator failed!
        return FDS_ERR_ARG;
    }

    // Add closing bracket for field
    ret_code = buffer_append(buffer,"]}");
    if (ret_code != FDS_OK){
        return ret_code;
    }

    return FDS_OK;
}

/**
 * \brief Append the buffer with a name of an Information Element
 *
 * Identification of the field is added in the long format i.e. '"<scope>:<id>":' or numeric
 * format '"enXXidYY":'. Please, note that the colon is part of the added string.
 * \note
 *   If the definition of a field is unknown or #FDS_CD2J_NUMERIC_ID flag is set,
 *   numeric identification is added.
 *
 * \param[in] buffer Buffer
 * \param[in] field Field identification to add
 * \return #FDS_OK on success
 * \return #FDS_ERR_NOMEM or #FDS_ERR_BUFFER in case of a memory allocation error
 */
int
add_field_name(struct context *buffer, const struct fds_drec_field *field)
{
    const struct fds_iemgr_elem *def = field->info->def;
    bool num_id = ((buffer->flags & FDS_CD2J_NUMERIC_ID) != 0);

    // If definition of field is unknown or if flag FDS_CD2J_NUMERIC_ID is set,
    // then identifier will be in format "enXX:idYY"
    if ((def == NULL) || (num_id)) {
        static const size_t scope_size = 32;
        char raw_name[scope_size];

        // Max length of identifier in format "enXX:idYY"
        // is "\"en" + 10x <en> + ":id" + 5x <id> + '\"\0'
        snprintf(raw_name, scope_size, "\"en%" PRIu32 ":id%" PRIu16 "\":", field->info->en,
            field->info->id);

        int ret_code = buffer_append(buffer, raw_name);
        if (ret_code != FDS_OK) {
            return ret_code;
        }
        return FDS_OK;
    }

    // Add a string identifier
    const size_t scope_size = strlen(def->scope->name);
    const size_t elem_size = strlen(def->name);

    size_t size = scope_size + elem_size + 5; // 2x '"' + 2x ':' + '\0'
    int ret_code = buffer_reserve(buffer, buffer_used(buffer) + size);
    if (ret_code != FDS_OK) {
        return ret_code;
    }
    *(buffer->write_begin++) = '"';
    memcpy(buffer->write_begin, def->scope->name, scope_size);
    buffer->write_begin += scope_size;

    *(buffer->write_begin++) = ':';
    memcpy(buffer->write_begin, def->name, elem_size);
    buffer->write_begin += elem_size;
    *(buffer->write_begin++) = '"';
    *(buffer->write_begin++) = ':';

    return FDS_OK;
}

int
fds_drec2json(const struct fds_drec *rec, uint32_t flags, const fds_iemgr_t *ie_mgr, char **str,
    size_t *str_size)
{
    // Allocate a memory if necessary
    bool null_buffer = false;
    if (*str == NULL) {
        null_buffer = true;
        *str = malloc(BUFFER_BASE);
        if (*str == NULL) {
            return FDS_ERR_NOMEM;
        }

        *str_size = BUFFER_BASE;
        flags |= FDS_CD2J_ALLOW_REALLOC;

    }

    // Prepare a conversion buffer
    struct context record;
    record.buffer_begin = *str;
    record.buffer_end = *str + *str_size;
    record.write_begin = record.buffer_begin;
    record.allow_real = ((flags & FDS_CD2J_ALLOW_REALLOC) != 0);
    record.flags = flags;
    record.mgr = ie_mgr;
    record.snap = rec->snap;

    // Convert the record
    int ret_code;
    if (rec->tmplt->type == FDS_TYPE_TEMPLATE_OPTS) {
        ret_code = buffer_append(&record,"{\"@type\":\"ipfix.optionsEntry\",");
    } else {
        ret_code = buffer_append(&record, "{\"@type\":\"ipfix.entry\",");
    }
    if (ret_code != FDS_OK) {
        goto error;
    }

    ret_code = iter_loop(rec, &record);
    if (ret_code != FDS_OK) {
        goto error;
    }

    ret_code = buffer_append(&record,"}"); // This also adds '\0'
    if(ret_code != FDS_OK) {
        goto error;
    }

    // Update values (buffer might be reallocated i.e. new pointer)
    *str = record.buffer_begin;
    *str_size = buffer_alloc(&record);

    return buffer_used(&record);

error:
    if (null_buffer) {
        free(str);
    }
    return ret_code;
}
