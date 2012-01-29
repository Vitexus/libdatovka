#include "isds_priv.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <iconv.h>
#include <langinfo.h>
#include <time.h>
#include <errno.h>
#include "utils.h"
#include "cencode.h"
#include "cdecode.h"

char *tz_orig; /* Copy of original TZ variable */

/* Concatenate two strings into newly allocated buffer.
 * You must free() them, when you don't need it anymore.
 * Any of the arguments can be NULL meaning empty string.
 * In case of error returns NULL.
 * Empty string is always returned as allocated empty string. */
_hidden char *_isds_astrcat(const char *first, const char *second) {
    size_t first_len, second_len;
    char *buf;
    
    first_len = (first) ? strlen(first) : 0;
    second_len = (second) ? strlen(second) : 0;
    buf = malloc(1 + first_len + second_len);
    if (buf) {
        buf[0] = '\0';
        if (first) strcpy(buf, first);
        if (second) strcpy(buf + first_len, second);
    }
    return buf;
}


/* Concatenate three strings into newly allocated buffer.
 * You must free() them, when you don't need it anymore.
 * Any of the arguments can be NULL meaning empty string.
 * In case of error returns NULL.
 * Empty string is always returned as allocated empty string. */
_hidden char *_isds_astrcat3(const char *first, const char *second,
        const char *third) {
    size_t first_len, second_len, third_len;
    char *buf, *next;
    
    first_len = (first) ? strlen(first) : 0;
    second_len = (second) ? strlen(second) : 0;
    third_len = (third) ? strlen(third) : 0;
    buf = malloc(1 + first_len + second_len + third_len);
    if (buf) {
        buf[0] = '\0';
        next = buf;
        if (first) {
            strcpy(next, first);
            next += first_len;
        }
        if (second) {
            strcpy(next, second);
            next += second_len;
        }
        if (third) {
            strcpy(next, third);
        }
    }
    return buf;
}


/* Print formatted string into automatically reallocated @buffer.
 * @buffer automatically reallocated buffer. Must be &NULL or preallocated
 * memory.
 * @format format string as for printf(3)
 * @ap list of variadic arguments, after call will be in undefined state
 * @Returns number of bytes printed. In case of error, -1 and NULL @buffer*/
_hidden int isds_vasprintf(char **buffer, const char *format, va_list ap) {
    va_list aq;
    int length, new_length;
    char *new_buffer;

    if (!buffer || !format) {
        if (buffer) {
            free(*buffer);
            *buffer = NULL;
        }
        return -1;
    }

    va_copy(aq, ap);
    length = vsnprintf(NULL, 0, format, aq) + 1;
    va_end(aq);
    if (length <= 0) {
        free(*buffer);
        *buffer = NULL;
        return -1;
    }

    new_buffer = realloc(*buffer, length);
    if (!new_buffer) {
        free(*buffer);
        *buffer = NULL;
        return -1;
    }
    *buffer = new_buffer;

    new_length = vsnprintf(*buffer, length, format, ap);
    if (new_length >= length) {
        free(*buffer);
        *buffer = NULL;
        return -1;
    }

    return new_length;
}


/* Print formatted string into automatically reallocated @buffer.
 * @buffer automatically reallocated buffer. Must be &NULL or preallocated
 * memory.
 * @format format string as for printf(3)
 * @... variadic arguments
 * @Returns number of bytes printed. In case of error, -1 and NULL @buffer*/
_hidden int isds_asprintf(char **buffer, const char *format, ...) {
    int ret;
    va_list ap;
    va_start(ap, format);
    ret = isds_vasprintf(buffer, format, ap);
    va_end(ap);
    return ret;
}


/* Converts UTF8 string into locale encoded string.
 * @utf string int UTF-8 terminated by zero byte
 * @return allocated string encoded in locale specific encoding. You must free
 * it. In case of error or NULL @utf returns NULL. */
_hidden char *_isds_utf82locale(const char *utf) {
    iconv_t state;
    size_t utf_length;
    char *buffer = NULL, *new_buffer;
    size_t buffer_length = 0, buffer_used = 0;
    char *inbuf, *outbuf;
    size_t inleft, outleft;

    if (!utf) return NULL;

    /* nl_langinfo() is not thread-safe */
    state = iconv_open(nl_langinfo(CODESET), "UTF-8");
    if (state == (iconv_t) -1) return NULL;

    /* Get the initial output buffer length */
    utf_length = strlen(utf);
    buffer_length = utf_length + 1;

    inbuf = (char *) utf;
    inleft = utf_length + 1;

    while (inleft > 0) {
        /* Extend buffer */
        new_buffer = realloc(buffer, buffer_length);
        if (!new_buffer) {
            free(buffer);
            buffer = NULL;
            goto leave;
        }
        buffer = new_buffer;

        /* FIXME */
        outbuf = buffer + buffer_used;
        outleft = buffer_length - buffer_used;
        
        /* Convert chunk of data */
        if ((size_t) -1 == iconv(state, &inbuf, &inleft, &outbuf, &outleft) &&
                errno != E2BIG) {
            free(buffer);
            buffer = NULL;
            goto leave;
        }

        /* Update positions */
        buffer_length += 1024;
        buffer_used = outbuf - buffer;
    }

leave:
    iconv_close(state);
    return buffer;
}


/* Encode given data into MIME Base64 encoded zero terminated string.
 * @plain are input data (binary stream)
 * @length is length of @plain data in bytes
 * @return allocated string of base64 encoded plain data or NULL in case of
 * error. You must free it. */
_hidden char *_isds_b64encode(const void *plain, const size_t length) {

    base64_encodestate state;
    size_t code_length;
    char *buffer, *new_buffer;

    if (!plain) {
        if (length) return NULL;
        /* Empty input is valid input */
        plain = "";
    }

    _isds_base64_init_encodestate(&state);

    /* Allocate buffer
     * (4 is padding, 1 is final new line, and 1 is string terminator) */
    buffer = malloc(length * 2 + 4 + 1 + 1);
    if (!buffer) return NULL;

    /* Encode plain data */
    code_length = _isds_base64_encode_block(plain, length, (int8_t *)buffer,
            &state);
    code_length += _isds_base64_encode_blockend(((int8_t*)buffer) + code_length,
            &state);

    /* Terminate string */
    buffer[code_length++] = '\0';

    /* Shrink the buffer */
    new_buffer = realloc(buffer, code_length);
    if (new_buffer) buffer = new_buffer;

    return buffer;
}


/* Decode given data from MIME Base64 encoded zero terminated string to binary
 * stream. Invalid Base64 symbols are skipped.
 * @encoded are input data (Base64 zero terminated string)
 * @plain are automatically reallocated output data (binary stream). You must
 * free it. Will be freed in case of error.
 * @return length of @plain data in bytes or (size_t) -1 in case of memory
 * allocation failure. */
_hidden size_t _isds_b64decode(const char *encoded, void **plain) {

    base64_decodestate state;
    size_t encoded_length;
    int plain_length;
    char *buffer;

    if (!encoded || !plain) {
        if (plain && *plain) zfree(*plain);
        return ((size_t) -1);
    }

    encoded_length = strlen(encoded);
    _isds_base64_init_decodestate(&state);

    /* Divert empty input */
    if (encoded_length == 0) {
        zfree(*plain);
        return 0;
    }

    /* Allocate buffer */
    buffer = realloc(*plain, encoded_length);
    if (!buffer) {
        zfree(*plain);
        return ((size_t) -1);
    }
    *plain = buffer;

    /* Decode encoded data */
    plain_length = _isds_base64_decode_block((const int8_t *)encoded,
            encoded_length, *plain, &state);
    if (plain_length < 0 || plain_length >= (size_t) -1) {
        zfree(*plain);
        return((size_t) -1);
    }

    /* Shrink the buffer */
    buffer = realloc(*plain, plain_length);
    if (!buffer) *plain = buffer;
    /* realloc(, 0) can return NULL or pointer designed to free() */
    if (plain_length == 0) zfree(*plain);

    return plain_length;
}


/* Switches time zone to UTC.
 * XXX: This is not reentrant and not thread-safe */
static void _isds_switch_tz_to_utc(void) {
    char *tz;

    tz = getenv("TZ");
    if (tz) {
        tz_orig = strdup(tz);
        if (!tz_orig) 
            PANIC("Can not back original time zone up");
    } else {
        tz_orig = NULL;
    }

    if (setenv("TZ", "", 1))
            PANIC("Can not change time zone to UTC temporarily");

    tzset();
}


/* Switches time zone to original value.
 * XXX: This is not reentrant and not thread-safe */
static void _isds_switch_tz_to_native(void) {
    if (tz_orig) {
        if (setenv("TZ", tz_orig, 1))
            PANIC("Can not restore time zone by setting TZ variable");
        free(tz_orig);
        tz_orig = NULL;
    } else {
        if(unsetenv("TZ"))
            PANIC("Can not restore time zone by unsetting TZ variable");
    }
    tzset();
}


/* Convert UTC broken time to time_t.
 * @broken_utc it time in UTC in broken format. Despite its content is not
 * touched, it'sw not-const because underlying POSIX function has non-const
 * signature.
 * @return (time_t) -1 in case of error */
_hidden time_t _isds_timegm(struct tm *broken_utc) {
    time_t time; 

    _isds_switch_tz_to_utc();
    time = mktime(broken_utc);
    _isds_switch_tz_to_native();

    return time;
}
