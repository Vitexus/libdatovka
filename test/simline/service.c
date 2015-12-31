#include "../../config.h"
#define _XOPEN_SOURCE XOPEN_SOURCE_LEVEL_FOR_STRDUP
#include "../test-tools.h"
#include "http.h"
#include "services.h"
#include "system.h"
#include <string.h>
#include <stdint.h>     /* For intmax_t */
#include <inttypes.h>   /* For PRIdMAX */
#include <ctype.h>      /* for isdigit() */
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/xmlsave.h>

static const char *soap_mime_type = "text/xml"; /* SOAP/1.1 requires text/xml */

/* Used to choose proper name space for message elements.
 * See _isds_register_namespaces(). */
typedef enum {
    MESSAGE_NS_1,
    MESSAGE_NS_UNSIGNED,
    MESSAGE_NS_SIGNED_INCOMING,
    MESSAGE_NS_SIGNED_OUTGOING,
    MESSAGE_NS_SIGNED_DELIVERY,
    MESSAGE_NS_OTP
} message_ns_type;

#define SOAP_NS "http://schemas.xmlsoap.org/soap/envelope/"
#define SOAP2_NS "http://www.w3.org/2003/05/soap-envelope"
#define ISDS1_NS "http://isds.czechpoint.cz"
#define ISDS_NS "http://isds.czechpoint.cz/v20"
#define OISDS_NS "http://isds.czechpoint.cz/v20/asws"
#define SISDS_INCOMING_NS "http://isds.czechpoint.cz/v20/message"
#define SISDS_OUTGOING_NS "http://isds.czechpoint.cz/v20/SentMessage"
#define SISDS_DELIVERY_NS "http://isds.czechpoint.cz/v20/delivery"
#define SCHEMA_NS "http://www.w3.org/2001/XMLSchema"
#define DEPOSIT_NS "urn:uschovnaWSDL"


struct service {
    service_id id;
    const char *end_point;
    const xmlChar *name_space;
    const xmlChar *name;
    http_error (*function) (
            xmlXPathContextPtr, xmlNodePtr,
            const void *arguments);
};


/* Convert UTF-8 ISO 8601 date-time @string to struct timeval.
 * It respects microseconds too. Microseconds are rounded half up.
 * In case of error, @time will be freed. */
static http_error timestring2timeval(const char *string,
        struct timeval **time) {
    struct tm broken;
    char *offset, *delim, *endptr;
    const int subsecond_resolution = 6;
    char subseconds[subsecond_resolution + 1];
    _Bool round_up = 0;
    int offset_hours, offset_minutes;
    int i;
    long int long_number;
#ifdef _WIN32
    int tmp;
#endif
    
    if (!time) return HTTP_ERROR_SERVER;
    if (!string) {
        free(*time);
        *time = NULL;
        return HTTP_ERROR_CLIENT;
    }

    memset(&broken, 0, sizeof(broken));

    if (!*time) {
        *time = calloc(1, sizeof(**time));
        if (!*time) return HTTP_ERROR_SERVER;
    } else {
        memset(*time, 0, sizeof(**time));
    }


    /* xsd:date is ISO 8601 string, thus ASCII */
    /*TODO: negative year */

#ifdef _WIN32
    i = 0;
    if ((tmp = sscanf((const char*)string, "%d-%d-%dT%d:%d:%d%n",
        &broken.tm_year, &broken.tm_mon, &broken.tm_mday,
        &broken.tm_hour, &broken.tm_min, &broken.tm_sec,
        &i)) < 6) {
        free(*time);
        *time = NULL;
        return HTTP_ERROR_CLIENT;
    }

    broken.tm_year -= 1900;
    broken.tm_mon--;
    broken.tm_isdst = -1;
    offset = (char*)string + i;
#else
    /* Parse date and time without subseconds and offset */
    offset = strptime((char*)string, "%Y-%m-%dT%T", &broken);
    if (!offset) {
        free(*time);
        *time = NULL;
        return HTTP_ERROR_CLIENT;
    }
#endif
    
    /* Get subseconds */
    if (*offset == '.' ) {
        offset++;

        /* Copy first 6 digits, pad it with zeros.
         * Current server implementation uses only millisecond resolution. */
        /* TODO: isdigit() is locale sensitive */
        for (i = 0;
                i < subsecond_resolution && isdigit(*offset);
                i++, offset++) {
            subseconds[i] = *offset;
        }
        if (subsecond_resolution == i && isdigit(*offset)) {
            /* Check 7th digit for rounding */
            if (*offset >= '5') round_up = 1;
            offset++;
        }
        for (; i < subsecond_resolution; i++) {
            subseconds[i] = '0';
        }
        subseconds[subsecond_resolution] = '\0';

        /* Convert it into integer */
        long_number = strtol(subseconds, &endptr, 10);
        if (*endptr != '\0' || long_number == LONG_MIN ||
                long_number == LONG_MAX) {
            free(*time);
            *time = NULL;
            return HTTP_ERROR_SERVER;
        }
        /* POSIX sys_time.h(0p) defines tv_usec timeval member as su_seconds_t
         * type. sys_types.h(0p) defines su_seconds_t as "used for time in
         * microseconds" and "the type shall be a signed integer capable of
         * storing values at least in the range [-1, 1000000]. */
        if (long_number < -1 || long_number >= 1000000) {
            free(*time);
            *time = NULL;
            return HTTP_ERROR_CLIENT;
        }
        (*time)->tv_usec = long_number;

        /* Round the subseconds */
        if (round_up) {
            if (999999 == (*time)->tv_usec) {
                (*time)->tv_usec = 0;
                broken.tm_sec++;
            } else {
                (*time)->tv_usec++;
            }
        }

        /* move to the zone offset delimiter or signal NULL*/
        delim = strchr(offset, '-');
        if (!delim)
            delim = strchr(offset, '+');
        if (!delim)
            delim = strchr(offset, 'Z');
        offset = delim;
    }

    /* Get zone offset */
    /* ISO allows zone offset string only: "" | "Z" | ("+"|"-" "<HH>:<MM>")
     * "" equals to "Z" and it means UTC zone. */
    /* One can not use strptime(, "%z",) becase it's RFC E-MAIL format without
     * colon separator */
    if (offset && (*offset == '-' || *offset == '+')) {
        if (2 != sscanf(offset + 1, "%2d:%2d", &offset_hours, &offset_minutes)) {
            free(*time);
            *time = NULL;
            return HTTP_ERROR_CLIENT;
        }
        if (*offset == '+') {
            broken.tm_hour -= offset_hours;
            broken.tm_min -= offset_minutes;
        } else {
            broken.tm_hour += offset_hours;
            broken.tm_min += offset_minutes;
        }
    }

    /* Convert to time_t */
    (*time)->tv_sec = _isds_timegm(&broken);
    if ((*time)->tv_sec == (time_t) -1) {
        free(*time);
        *time = NULL;
        return HTTP_ERROR_CLIENT;
    }

    return HTTP_ERROR_SUCCESS;
}


/* Following EXTRACT_* macros expect @xpath_ctx, @error, @message,
 * and leave label. */
#define ELEMENT_EXISTS(element, allow_multiple) { \
    xmlXPathObjectPtr result = NULL; \
    result = xmlXPathEvalExpression(BAD_CAST element, xpath_ctx); \
    if (NULL == result) { \
        error = HTTP_ERROR_SERVER; \
        goto leave; \
    } \
    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) { \
            xmlXPathFreeObject(result); \
            test_asprintf(&message, "Element %s does not exist", element); \
            error = HTTP_ERROR_CLIENT; \
            goto leave; \
    } else { \
        if (!allow_multiple && result->nodesetval->nodeNr > 1) { \
            xmlXPathFreeObject(result); \
            test_asprintf(&message, "Multiple %s element", element); \
            error = HTTP_ERROR_CLIENT; \
            goto leave; \
        } \
    } \
    xmlXPathFreeObject(result); \
}

#define EXTRACT_STRING(element, string) { \
    xmlXPathObjectPtr result = NULL; \
    result = xmlXPathEvalExpression(BAD_CAST element "/text()", xpath_ctx); \
    if (NULL == result) { \
        error = HTTP_ERROR_SERVER; \
        goto leave; \
    } \
    if (!xmlXPathNodeSetIsEmpty(result->nodesetval)) { \
        if (result->nodesetval->nodeNr > 1) { \
            xmlXPathFreeObject(result); \
            test_asprintf(&message, "Multiple %s element", element); \
            error = HTTP_ERROR_CLIENT; \
            goto leave; \
        } \
        (string) = (char *) \
            xmlXPathCastNodeSetToString(result->nodesetval); \
        if (!(string)) { \
            xmlXPathFreeObject(result); \
            error = HTTP_ERROR_SERVER; \
            goto leave; \
        } \
    } \
    xmlXPathFreeObject(result); \
}

#define EXTRACT_BOOLEAN(element, booleanPtr) { \
    char *string = NULL; \
    EXTRACT_STRING(element, string); \
     \
    if (NULL != string) { \
        (booleanPtr) = calloc(1, sizeof(*(booleanPtr))); \
        if (NULL == (booleanPtr)) { \
            free(string); \
            error = HTTP_ERROR_SERVER; \
            goto leave; \
        } \
         \
        if (!xmlStrcmp((xmlChar *)string, BAD_CAST "true") || \
                !xmlStrcmp((xmlChar *)string, BAD_CAST "1")) \
            *(booleanPtr) = 1; \
        else if (!xmlStrcmp((xmlChar *)string, BAD_CAST "false") || \
                !xmlStrcmp((xmlChar *)string, BAD_CAST "0")) \
            *(booleanPtr) = 0; \
        else { \
            test_asprintf(&message, \
                    "%s value is not valid boolean: %s", \
                    element, string); \
            free(string); \
            error = HTTP_ERROR_CLIENT; \
            goto leave; \
        } \
         \
        free(string); \
    } \
} 


#define EXTRACT_DATE(element, tmPtr) { \
    char *string = NULL; \
    EXTRACT_STRING(element, string); \
    if (NULL != string) { \
        (tmPtr) = calloc(1, sizeof(*(tmPtr))); \
        if (NULL == (tmPtr)) { \
            free(string); \
            error = HTTP_ERROR_SERVER; \
            goto leave; \
        } \
        error = _server_datestring2tm(string, (tmPtr)); \
        if (error) { \
            if (error == HTTP_ERROR_CLIENT) { \
                test_asprintf(&message, "%s value is not a valid date: %s", \
                        element, string); \
            } \
            free(string); \
            goto leave; \
        } \
        free(string); \
    } \
}

/* Following INSERT_* macros expect @error and leave label */
#define INSERT_STRING_WITH_NS(parent, ns, element, string) \
    { \
        xmlNodePtr node = xmlNewTextChild(parent, ns, BAD_CAST (element), \
                (xmlChar *) (string)); \
        if (NULL == node) { \
            error = HTTP_ERROR_SERVER; \
            goto leave; \
        } \
    }

#define INSERT_STRING(parent, element, string) \
    { INSERT_STRING_WITH_NS(parent, NULL, element, string) }

#define INSERT_LONGINTPTR(parent, element, longintPtr) { \
    if ((longintPtr)) { \
        char *buffer = NULL; \
        /* FIXME: locale sensitive */ \
        if (-1 == test_asprintf(&buffer, "%ld", *(longintPtr))) { \
            error = HTTP_ERROR_SERVER; \
            goto leave; \
        } \
        INSERT_STRING(parent, element, buffer) \
        free(buffer); \
    } else { INSERT_STRING(parent, element, NULL) } \
}

#define INSERT_ULONGINTPTR(parent, element, ulongintPtr) { \
    if ((ulongintPtr)) { \
        char *buffer = NULL; \
        /* FIXME: locale sensitive */ \
        if (-1 == test_asprintf(&buffer, "%lu", *(ulongintPtr))) { \
            error = HTTP_ERROR_SERVER; \
            goto leave; \
        } \
        INSERT_STRING(parent, element, buffer) \
        free(buffer); \
    } else { INSERT_STRING(parent, element, NULL) } \
}

#define INSERT_BOOLEANPTR(parent, element, booleanPtr) { \
    if (NULL != (booleanPtr)) { \
        char *buffer = NULL; \
        buffer = *(booleanPtr) ? "true" : "false"; \
        INSERT_STRING(parent, element, buffer) \
    } else { INSERT_STRING(parent, element, NULL) } \
}

#define INSERT_TIMEVALPTR(parent, element, timevalPtr) { \
    if (NULL != (timevalPtr)) { \
        char *buffer = NULL; \
        error = timeval2timestring(timevalPtr, &buffer); \
        if (error) { \
            free(buffer); \
            goto leave; \
        } \
        INSERT_STRING(parent, element, buffer); \
        free(buffer); \
    } else { \
        INSERT_STRING(parent, element, NULL); \
    } \
}

#define INSERT_TMPTR(parent, element, tmPtr) { \
    if (NULL != (tmPtr)) { \
        char *buffer = NULL; \
        error = tm2datestring(tmPtr, &buffer); \
        if (error) { \
            free(buffer); \
            goto leave; \
        } \
        INSERT_STRING(parent, element, buffer); \
        free(buffer); \
    } else { \
        INSERT_STRING(parent, element, NULL); \
    } \
}

#define INSERT_ELEMENT(child, parent, element) \
    { \
        (child) = xmlNewChild((parent), NULL, BAD_CAST (element), NULL); \
        if (NULL == (child)) { \
            error = HTTP_ERROR_SERVER; \
            goto leave; \
        } \
    }

/* TODO: These functions (element_exists(), extract_...(), ...) will replace
 * the macros (ELEMENT_EXISTS, EXTRACT_...). The compiled code will be
 * smaller, the compilation will be faster. */

/* Check an element exists.
 * @code is a static output ISDS error code
 * @error_message is a reallocated output ISDS error message
 * @xpath_ctx is a current XPath context
 * @element_name is name of an element to check
 * @allow_multiple is false to require exactly one element. True to require
 * one or more elements.
 * @return HTTP_ERROR_SUCCESS or an appropriate error code. */
static http_error element_exists(const char **code, char **message,
        xmlXPathContextPtr xpath_ctx, const char *element_name,
        _Bool allow_multiple) {
    xmlXPathObjectPtr result = NULL;

    result = xmlXPathEvalExpression(BAD_CAST element_name, xpath_ctx);
    if (NULL == result) {
        return HTTP_ERROR_SERVER;
    }
    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
            xmlXPathFreeObject(result);
            *code = "9999";
            test_asprintf(message, "Element %s does not exist", element_name);
            return HTTP_ERROR_CLIENT;
    } else {
        if (!allow_multiple && result->nodesetval->nodeNr > 1) {
            xmlXPathFreeObject(result);
            *code = "9999";
            test_asprintf(message, "Multiple %s element", element_name);
            return HTTP_ERROR_CLIENT;
        }
    }
    xmlXPathFreeObject(result);

    return HTTP_ERROR_SUCCESS;
}


/* Locate a children element.
 * @code is a static output ISDS error code
 * @error_message is a reallocated output ISDS error message
 * @xpath_ctx is a current XPath context
 * @element_name is name of an element to select
 * @node is output pointer to located element node
 * @return HTTP_ERROR_SUCCESS or an appropriate error code. */
static http_error select_element(const char **code, char **message,
        xmlXPathContextPtr xpath_ctx, const char *element_name,
        xmlNodePtr *node) {
    xmlXPathObjectPtr result = NULL;

    result = xmlXPathEvalExpression(BAD_CAST element_name, xpath_ctx);
    if (NULL == result) {
        return HTTP_ERROR_SERVER;
    }
    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
            xmlXPathFreeObject(result);
            *code = "9999";
            test_asprintf(message, "Element %s does not exist", element_name);
            return HTTP_ERROR_CLIENT;
    } else {
        if (result->nodesetval->nodeNr > 1) {
            xmlXPathFreeObject(result);
            *code = "9999";
            test_asprintf(message, "Multiple %s element", element_name);
            return HTTP_ERROR_CLIENT;
        }
    }
    *node = result->nodesetval->nodeTab[0];
    xmlXPathFreeObject(result);

    return HTTP_ERROR_SUCCESS;
}


/* Extract @element_name's value as a string.
 * @code is a static output ISDS error code
 * @error_message is a reallocated output ISDS error message
 * @xpath_ctx is a current XPath context
 * @element_name is name of a element whose child text node to extract
 * @string is the extraced allocated string value, or NULL if empty or the
 * element does not exist.
 * @return HTTP_ERROR_SUCCESS or an appropriate error code. */
static http_error extract_string(const char **code, char **message,
        xmlXPathContextPtr xpath_ctx, const char *element_name,
        char **string) {
    http_error error = HTTP_ERROR_SUCCESS;
    xmlXPathObjectPtr result = NULL;
    char *buffer = NULL;

    if (-1 == test_asprintf(&buffer, "%s/text()", element_name)) {
        error = HTTP_ERROR_SERVER;
        goto leave;
    }
    result = xmlXPathEvalExpression(BAD_CAST buffer, xpath_ctx);
    free(buffer);
    if (NULL == result) {
        error = HTTP_ERROR_SERVER;
        goto leave;
    }
    if (!xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        if (result->nodesetval->nodeNr > 1) {
            xmlXPathFreeObject(result);
            *code = "9999";
            test_asprintf(message, "Multiple %s element", element_name);
            error = HTTP_ERROR_CLIENT;
            goto leave;
        }
        *string = (char *) \
            xmlXPathCastNodeSetToString(result->nodesetval);
        if (!(*string)) {
            xmlXPathFreeObject(result);
            error = HTTP_ERROR_SERVER;
            goto leave;
        }
    }
    xmlXPathFreeObject(result);

leave:
    return error;
}


/* Compare dates represented by pointer to struct tm.
 * @return 0 if equalued, non-0 otherwise. */
static int datecmp(const struct tm *a, const struct tm *b) {
    if (NULL == a && b == NULL) return 0;
    if ((NULL == a && b != NULL) || (NULL != a && b == NULL)) return 1;
    if (a->tm_year != b->tm_year) return 1;
    if (a->tm_mon != b->tm_mon) return 1;
    if (a->tm_mday != b->tm_mday) return 1;
    return 0;
}


/* Compare times represented by pointer to struct timeval.
 * @return 0 if equalued, non-0 otherwise. */
static int timecmp(const struct timeval *a, const struct timeval *b) {
    if (NULL == a && b == NULL) return 0;
    if ((NULL == a && b != NULL) || (NULL != a && b == NULL)) return 1;
    if (a->tv_sec != b->tv_sec) return 1;
    if (a->tv_usec != b->tv_usec) return 1;
    return 0;
}


/* Checks an @element_name's value is an @expected_value string.
 * @code is a static output ISDS error code
 * @error_message is a reallocated output ISDS error message
 * @xpath_ctx is a current XPath context
 * @element_name is name of a element to check
 * @must_exist is true if the @element_name must exist even if @expected_value
 * is NULL.
 * @expected_value is an expected string value
 * @return HTTP_ERROR_SUCCESS if the @element_name element's value is
 * @expected_value. HTTP_ERROR_CLIENT if not equaled, HTTP_ERROR_SERVER if an
 * internal error occured. */
static http_error element_equals_string(const char **code, char **message,
        xmlXPathContextPtr xpath_ctx, const char *element_name,
        _Bool must_exist, const char *expected_value) {
    http_error error = HTTP_ERROR_SUCCESS;
    char *string = NULL;

    if (must_exist) {
        error = element_exists(code, message, xpath_ctx, element_name, 0);
        if (HTTP_ERROR_SUCCESS != error)
            goto leave;
    }

    error = extract_string(code, message, xpath_ctx, element_name, &string);
    if (HTTP_ERROR_SUCCESS != error)
        goto leave;

    if (NULL != expected_value) {
        if (NULL == string) {
            *code = "9999";
            test_asprintf(message, "Empty %s element", element_name);
            error = HTTP_ERROR_CLIENT;
            goto leave;
        }
        if (xmlStrcmp(BAD_CAST expected_value, BAD_CAST string)) {
            *code = "9999";
            test_asprintf(message,
                    "Unexpected %s element value: expected=`%s', got=`%s'",
                    element_name, expected_value, string);
            error = HTTP_ERROR_CLIENT;
            goto leave;
        }
    } else {
        if (NULL != string && *string != '\0') {
            *code = "9999";
            test_asprintf(message,
                    "Unexpected %s element value: "
                    "expected empty string, got=`%s'",
                    element_name, string);
            error = HTTP_ERROR_CLIENT;
            goto leave;
        }
    }

leave:
    free(string);
    return error;
}


/* Checks an @element_name's value is an @expected_value integer.
 * @code is a static output ISDS error code
 * @error_message is a reallocated output ISDS error message
 * @xpath_ctx is a current XPath context
 * @element_name is name of a element to check
 * @must_exist is true if the @element_name must exist even if @expected_value
 * is NULL.
 * @expected_value is an expected integer value.
 * @return HTTP_ERROR_SUCCESS if the @element_name element's value is
 * @expected_value. HTTP_ERROR_CLIENT if not equaled, HTTP_ERROR_SERVER if an
 * internal error occured. */
static http_error element_equals_integer(const char **code, char **message,
        xmlXPathContextPtr xpath_ctx, const char *element_name,
        _Bool must_exist, const long int *expected_value) {
    http_error error = HTTP_ERROR_SUCCESS;
    char *string = NULL;
    long int number;
    char *endptr;

    if (must_exist) {
        error = element_exists(code, message, xpath_ctx, element_name, 0);
        if (HTTP_ERROR_SUCCESS != error)
            goto leave;
    }

    error = extract_string(code, message, xpath_ctx, element_name, &string);
    if (HTTP_ERROR_SUCCESS != error)
        goto leave;

    if (NULL != expected_value) {
        if (NULL == string) {
            *code = "9999";
            test_asprintf(message, "Empty %s element", element_name);
            error = HTTP_ERROR_CLIENT;
            goto leave;
        }
        number = strtol(string, &endptr, 10);
        if (*endptr != '\0') {
            *code = "9999";
            test_asprintf(message,
                    "%s element value is not a valid integer: %s",
                    element_name, string);
            error = HTTP_ERROR_CLIENT;
            goto leave;
        }
        if (number == LONG_MIN || number == LONG_MAX) { \
            *code = "9999";
            test_asprintf(message, \
                    "%s element value is out of range of long int: %s",
                    element_name, string);
            error = HTTP_ERROR_SERVER;
            goto leave;
        }
        free(string); string = NULL;
        if (number != *expected_value) {
            *code = "9999";
            test_asprintf(message,
                    "Unexpected %s element value: expected=`%ld', got=`%ld'",
                    element_name, *expected_value, number);
            error = HTTP_ERROR_CLIENT;
            goto leave;
        }
    } else {
        if (NULL != string && *string != '\0') {
            *code = "9999";
            test_asprintf(message,
                    "Unexpected %s element value: expected no text node, got=`%s'",
                    element_name, string);
            error = HTTP_ERROR_CLIENT;
            goto leave;
        }
    }

leave:
    free(string);
    return error;
}


/* Checks an @element_name's value is an @expected_value boolean.
 * @code is a static output ISDS error code
 * @error_message is an reallocated output ISDS error message
 * @xpath_ctx is a current XPath context
 * @element_name is name of a element to check
 * @must_exist is true if the @element_name must exist even if @expected_value
 * is NULL.
 * @expected_value is an expected boolean value
 * @return HTTP_ERROR_SUCCESS if the @element_name element's value is
 * @expected_value. HTTP_ERROR_CLIENT if not equaled, HTTP_ERROR_SERVER if an
 * internal error occured. */
static http_error element_equals_boolean(const char **code, char **message,
        xmlXPathContextPtr xpath_ctx, const char *element_name,
        _Bool must_exist, const _Bool *expected_value) {
    http_error error = HTTP_ERROR_SUCCESS;
    char *string = NULL;
    _Bool value;

    if (must_exist) {
        error = element_exists(code, message, xpath_ctx, element_name, 0);
        if (HTTP_ERROR_SUCCESS != error)
            goto leave;
    }

    error = extract_string(code, message, xpath_ctx, element_name, &string);
    if (HTTP_ERROR_SUCCESS != error)
        goto leave;

    if (NULL != expected_value) {
        if (NULL == string) {
            *code = "9999";
            test_asprintf(message, "Empty %s element", element_name);
            error = HTTP_ERROR_CLIENT;
            goto leave;
        }
        if (!xmlStrcmp((xmlChar *)string, BAD_CAST "true") ||
                !xmlStrcmp((xmlChar *)string, BAD_CAST "1"))
            value = 1;
        else if (!xmlStrcmp((xmlChar *)string, BAD_CAST "false") ||
                !xmlStrcmp((xmlChar *)string, BAD_CAST "0"))
            value = 0;
        else {
            *code = "9999";
            test_asprintf(message,
                    "%s element value is not a valid boolean: %s",
                    element_name, string);
            error = HTTP_ERROR_CLIENT;
            goto leave;
        }
        if (*expected_value != value) {
            *code = "9999";
            test_asprintf(message,
                    "Unexpected %s element value: expected=%d, got=%d",
                    element_name, *expected_value, string);
            error = HTTP_ERROR_CLIENT;
            goto leave;
        }
    } else {
        if (NULL != string && *string != '\0') {
            *code = "9999";
            test_asprintf(message,
                    "Unexpected %s element value: "
                    "expected empty string, got=`%s'",
                    element_name, string);
            error = HTTP_ERROR_CLIENT;
            goto leave;
        }
    }

leave:
    free(string);
    return error;
}


/* Checks an @element_name's value is an @expected_value date.
 * @code is a static output ISDS error code
 * @error_message is an reallocated output ISDS error message
 * @xpath_ctx is a current XPath context
 * @element_name is name of a element to check
 * @must_exist is true if the @element_name must exist even if @expected_value
 * is NULL.
 * @expected_value is an expected boolean value
 * @return HTTP_ERROR_SUCCESS if the @element_name element's value is
 * @expected_value. HTTP_ERROR_CLIENT if not equaled, HTTP_ERROR_SERVER if an
 * internal error occured. */
static http_error element_equals_date(const char **code, char **message,
        xmlXPathContextPtr xpath_ctx, const char *element_name,
        _Bool must_exist, const struct tm *expected_value) {
    http_error error = HTTP_ERROR_SUCCESS;
    char *string = NULL;
    struct tm value;

    if (must_exist) {
        error = element_exists(code, message, xpath_ctx, element_name, 0);
        if (HTTP_ERROR_SUCCESS != error)
            goto leave;
    }

    error = extract_string(code, message, xpath_ctx, element_name, &string);
    if (HTTP_ERROR_SUCCESS != error)
        goto leave;

    if (NULL != expected_value) {
        if (NULL == string) {
            *code = "9999";
            test_asprintf(message, "Empty %s element", element_name);
            error = HTTP_ERROR_CLIENT;
            goto leave;
        }
        error = _server_datestring2tm(string, &value);
        if (error) {
            if (error == HTTP_ERROR_CLIENT) { \
                test_asprintf(message, "%s value is not a valid date: %s",
                        element_name, string);
            }
            goto leave;
        }
        if (datecmp(expected_value, &value)) {
            *code = "9999";
            test_asprintf(message, "Unexpected %s element value: "
                    "expected=%d-%02d-%02d, got=%d-%02d-%02d", element_name,
                    expected_value->tm_year + 1900, expected_value->tm_mon + 1,
                    expected_value->tm_mday,
                    value.tm_year + 1900, value.tm_mon + 1, value.tm_mday);
            error = HTTP_ERROR_CLIENT;
            goto leave;
        }
    } else {
        if (NULL != string && *string != '\0') {
            *code = "9999";
            test_asprintf(message,
                    "Unexpected %s element value: "
                    "expected empty value, got=`%s'",
                    element_name, string);
            error = HTTP_ERROR_CLIENT;
            goto leave;
        }
    }

leave:
    free(string);
    return error;
}


/* Checks an @element_name's value is an @expected_value time.
 * @code is a static output ISDS error code
 * @error_message is an reallocated output ISDS error message
 * @xpath_ctx is a current XPath context
 * @element_name is name of a element to check
 * @must_exist is true if the @element_name must exist even if @expected_value
 * is NULL.
 * @expected_value is an expected boolean value
 * @return HTTP_ERROR_SUCCESS if the @element_name element's value is
 * @expected_value. HTTP_ERROR_CLIENT if not equaled, HTTP_ERROR_SERVER if an
 * internal error occured. */
static http_error element_equals_time(const char **code, char **message,
        xmlXPathContextPtr xpath_ctx, const char *element_name,
        _Bool must_exist, const struct timeval *expected_value) {
    http_error error = HTTP_ERROR_SUCCESS;
    char *string = NULL;
    struct timeval *value = NULL;

    if (must_exist) {
        error = element_exists(code, message, xpath_ctx, element_name, 0);
        if (HTTP_ERROR_SUCCESS != error)
            goto leave;
    }

    error = extract_string(code, message, xpath_ctx, element_name, &string);
    if (HTTP_ERROR_SUCCESS != error)
        goto leave;

    if (NULL != expected_value) {
        if (NULL == string) {
            *code = "9999";
            test_asprintf(message, "Empty %s element", element_name);
            error = HTTP_ERROR_CLIENT;
            goto leave;
        }
        error = timestring2timeval(string, &value);
        if (error) {
            if (error == HTTP_ERROR_CLIENT) { \
                test_asprintf(message, "%s value is not a valid time: %s",
                        element_name, string);
            }
            goto leave;
        }
        if (timecmp(expected_value, value)) {
            *code = "9999";
            test_asprintf(message, "Unexpected %s element value: "
                    "expected=%ds:%" PRIdMAX "us, got=%ds:%" PRIdMAX "us",
                    element_name,
                    expected_value->tv_sec, (intmax_t)expected_value->tv_usec,
                    value->tv_sec, (intmax_t)value->tv_usec);
            error = HTTP_ERROR_CLIENT;
            goto leave;
        }
    } else {
        if (NULL != string && *string != '\0') {
            *code = "9999";
            test_asprintf(message,
                    "Unexpected %s element value: "
                    "expected empty value, got=`%s'",
                    element_name, string);
            error = HTTP_ERROR_CLIENT;
            goto leave;
        }
    }

leave:
    free(string);
    free(value);
    return error;
}


/* Insert dmStatus or similar subtree
 * @parent is element to insert to
 * @dm is true for dmStatus, otherwise dbStatus
 * @code is status code as string
 * @message is UTF-8 encoded message
 * @db_ref_number is optinal reference number propagated if not @dm
 * @return 0 on success, otherwise non-0. */
static http_error insert_isds_status(xmlNodePtr parent, _Bool dm,
        const xmlChar *code, const xmlChar *message,
        const xmlChar *db_ref_number) {
    http_error error = HTTP_ERROR_SUCCESS;
    xmlNodePtr status;
    
    if (NULL == code || NULL == message) {
        error = HTTP_ERROR_SERVER;
        goto leave;
    }

    INSERT_ELEMENT(status, parent, (dm) ? "dmStatus" : "dbStatus");
    INSERT_STRING(status, (dm) ? "dmStatusCode" : "dbStatusCode", code);
    INSERT_STRING(status, (dm) ? "dmStatusMessage" : "dbStatusMessage", message);
    if (!dm && NULL != db_ref_number) {
        INSERT_STRING(status, "dbStatusRefNumber", db_ref_number);
    }

leave:
    return error;
}


/* Convert struct tm *@time to UTF-8 ISO 8601 date @string. */
static http_error tm2datestring(const struct tm *time, char **string) {
    if (NULL == time || NULL == string) return HTTP_ERROR_SERVER;

    if (-1 == test_asprintf(string, "%d-%02d-%02d",
                time->tm_year + 1900, time->tm_mon + 1, time->tm_mday))
        return HTTP_ERROR_SERVER;

    return HTTP_ERROR_SUCCESS;
}


/* Convert struct timeval *@time to UTF-8 ISO 8601 date-time @string. It
 * respects the @time microseconds too. */
static http_error timeval2timestring(const struct timeval *time,
        char **string) {
    struct tm broken;

    if (!time || !string) return HTTP_ERROR_SERVER;

    if (!gmtime_r(&time->tv_sec, &broken)) return HTTP_ERROR_SERVER;
    if (time->tv_usec < 0 || time->tv_usec > 999999) return HTTP_ERROR_SERVER;

    /* TODO: small negative year should be formatted as "-0012". This is not
     * true for glibc "%04d". We should implement it.
     * time->tv_usec type is su_seconds_t which is required to be signed
     * integer to accomodate values from range [-1, 1000000].
     * See <http://www.w3.org/TR/2001/REC-xmlschema-2-20010502/#dateTime> */ 
    if (-1 == test_asprintf(string,
                "%04d-%02d-%02dT%02d:%02d:%02d.%06" PRIdMAX,
                broken.tm_year + 1900, broken.tm_mon + 1, broken.tm_mday,
                broken.tm_hour, broken.tm_min, broken.tm_sec,
                (intmax_t)time->tv_usec))
        return HTTP_ERROR_SERVER;

    return HTTP_ERROR_SUCCESS;
}


/* Implement DummyOperation */
static http_error service_DummyOperation(
        xmlXPathContextPtr xpath_ctx,
        xmlNodePtr isds_response,
        const void *arguments) {
    (void)xpath_ctx;
    (void)arguments;

    return insert_isds_status(isds_response, 1, BAD_CAST "0000",
            BAD_CAST "Success", NULL);
}


/* Implement Re-signISDSDocument.
 * It sends document from request back.
 * @arguments is pointer to struct arguments_DS_Dz_ResignISDSDocument */
static http_error service_ResignISDSDocument(
        xmlXPathContextPtr xpath_ctx,
        xmlNodePtr isds_response,
        const void *arguments) {
    http_error error = HTTP_ERROR_SUCCESS;
    const char *code = "9999";
    char *message = NULL;
    const struct arguments_DS_Dz_ResignISDSDocument *configuration =
        (const struct arguments_DS_Dz_ResignISDSDocument *)arguments;
    char *data = NULL;

    if (NULL == configuration || NULL == configuration->status_code ||
            NULL == configuration->status_message) {
        error = HTTP_ERROR_SERVER;
        goto leave;
    }

    EXTRACT_STRING("isds:dmDoc", data);
    if (NULL == data) {
        message = strdup("Missing isds:dmDoc");
        error = HTTP_ERROR_CLIENT;
        goto leave;
    }


    /* dmResultDoc is mandatory in response */
    if (xmlStrcmp(BAD_CAST configuration->status_code, BAD_CAST "0000")) {
        free(data);
        data = NULL;
    }
    INSERT_STRING(isds_response, "dmResultDoc", data);

    if (configuration->valid_to != NULL) {
        error = tm2datestring(configuration->valid_to, &data);
        if (error) {
            message = strdup("Could not format date");
            goto leave;
        }
        INSERT_STRING(isds_response, "dmValidTo", data);
    }

    code = configuration->status_code;
    message = strdup(configuration->status_message);

leave:
    if (HTTP_ERROR_SERVER != error) {
        http_error next_error = insert_isds_status(isds_response, 1,
                BAD_CAST code, BAD_CAST message, NULL);
        if (HTTP_ERROR_SUCCESS != next_error) error = next_error;
    }
    free(data);
    free(message);
    return error;
}


/* Implement EraseMessage.
 * @arguments is pointer to struct arguments_DS_DsManage_ChangeISDSPassword */
static http_error service_EraseMessage(
        xmlXPathContextPtr xpath_ctx,
        xmlNodePtr isds_response,
        const void *arguments) {
    http_error error = HTTP_ERROR_SUCCESS;
    char *code = "9999", *message = NULL;
    const struct arguments_DS_Dx_EraseMessage *configuration =
        (const struct arguments_DS_Dx_EraseMessage *)arguments;
    char *message_id = NULL;
    _Bool *incoming = NULL;

    if (NULL == configuration || NULL == configuration->message_id) {
        error = HTTP_ERROR_SERVER;
        goto leave;
    }

    EXTRACT_STRING("isds:dmID", message_id);
    if (NULL == message_id) {
        message = strdup("Missing isds:dmID");
        error = HTTP_ERROR_CLIENT;
        goto leave;
    }
    EXTRACT_BOOLEAN("isds:dmIncoming", incoming);
    if (NULL == incoming) {
        message = strdup("Missing isds:dmIncoming");
        error = HTTP_ERROR_CLIENT;
        goto leave;
    }

    if (xmlStrcmp((const xmlChar *) configuration->message_id,
                (const xmlChar *) message_id)) {
        code = "1219";
        message = strdup("Message is not in the long term storage");
        error = HTTP_ERROR_CLIENT;
        goto leave;
    }
    if (configuration->incoming != *incoming) {
        code = "1219";
        message = strdup("Message direction mismatches");
        error = HTTP_ERROR_CLIENT;
        goto leave;
    }
    
    code = "0000";
    message = strdup("Success");
leave:
    if (HTTP_ERROR_SERVER != error) {
        http_error next_error = insert_isds_status(isds_response, 1,
                BAD_CAST code, BAD_CAST message, NULL);
        if (HTTP_ERROR_SUCCESS != next_error) error = next_error;
    }
    free(incoming);
    free(message_id);
    free(message);
    return error;
}


/* Insert list of credit info as XSD:tCiRecord XML tree.
 * @isds_response is XML node with the response
 * @history is list of struct server_credit_event. If NULL, no ciRecord XML
 * subtree will be created. */
static http_error insert_ciRecords(xmlNodePtr isds_response,
        const struct server_list *history) {
    http_error error = HTTP_ERROR_SUCCESS;
    xmlNodePtr records, record;

    if (NULL == isds_response) return HTTP_ERROR_SERVER;
    if (NULL == history) return HTTP_ERROR_SUCCESS;

    INSERT_ELEMENT(records, isds_response, "ciRecords");
    for (const struct server_list *item = history; NULL != item;
            item = item->next) {
        const struct server_credit_event *event =
            (struct server_credit_event*)item->data;
        
        INSERT_ELEMENT(record, records, "ciRecord");
        if (NULL == event) continue;

        INSERT_TIMEVALPTR(record, "ciEventTime", event->time);
        switch(event->type) {
            case SERVER_CREDIT_CHARGED:
                INSERT_STRING(record, "ciEventType", "1");
                INSERT_STRING(record, "ciTransID",
                        event->details.charged.transaction);
                break;
            case SERVER_CREDIT_DISCHARGED:
                INSERT_STRING(record, "ciEventType", "2");
                INSERT_STRING(record, "ciTransID",
                        event->details.discharged.transaction);
                break;
            case SERVER_CREDIT_MESSAGE_SENT:
                INSERT_STRING(record, "ciEventType", "3");
                INSERT_STRING(record, "ciRecipientID",
                        event->details.message_sent.recipient);
                INSERT_STRING(record, "ciPDZID",
                        event->details.message_sent.message_id);
                break;
            case SERVER_CREDIT_STORAGE_SET:
                INSERT_STRING(record, "ciEventType", "4");
                INSERT_LONGINTPTR(record, "ciNewCapacity",
                        &event->details.storage_set.new_capacity);
                INSERT_TMPTR(record, "ciNewFrom",
                        event->details.storage_set.new_valid_from);
                INSERT_TMPTR(record, "ciNewTo",
                        event->details.storage_set.new_valid_to);
                INSERT_LONGINTPTR(record, "ciOldCapacity",
                        event->details.storage_set.old_capacity);
                INSERT_TMPTR(record, "ciOldFrom",
                        event->details.storage_set.old_valid_from);
                INSERT_TMPTR(record, "ciOldTo",
                        event->details.storage_set.old_valid_to);
                INSERT_STRING(record, "ciDoneBy",
                        event->details.storage_set.initiator);
                break;
            case SERVER_CREDIT_EXPIRED:
                INSERT_STRING(record, "ciEventType", "5");
                break;
            default:
                error = HTTP_ERROR_SERVER;
                goto leave;
        }
        INSERT_LONGINTPTR(record, "ciCreditChange", &event->credit_change);
        INSERT_LONGINTPTR(record, "ciCreditAfter", &event->new_credit);

    }

leave:
    return error;
}


/* Implement DataBoxCreditInfo.
 * @arguments is pointer to struct arguments_DS_df_DataBoxCreditInfo */
static http_error service_DataBoxCreditInfo(
        xmlXPathContextPtr xpath_ctx,
        xmlNodePtr isds_response,
        const void *arguments) {
    http_error error = HTTP_ERROR_SUCCESS;
    const char *code = "9999";
    char *message = NULL;
    const struct arguments_DS_df_DataBoxCreditInfo *configuration =
        (const struct arguments_DS_df_DataBoxCreditInfo *)arguments;
    char *box_id = NULL;
    struct tm *from_date = NULL, *to_date = NULL;

    if (NULL == configuration || NULL == configuration->status_code ||
            NULL == configuration->status_message) {
        error = HTTP_ERROR_SERVER;
        goto leave;
    }

    EXTRACT_STRING("isds:dbID", box_id);
    if (NULL == box_id) {
        message = strdup("Missing isds:dbID");
        error = HTTP_ERROR_CLIENT;
        goto leave;
    }
    if (NULL != configuration->box_id &&
            xmlStrcmp(BAD_CAST configuration->box_id,
                    BAD_CAST box_id)) {
        code = "9999";
        message = strdup("Unexpected isds:dbID value");
        error = HTTP_ERROR_CLIENT;
        goto leave;
    }
    
    ELEMENT_EXISTS("isds:ciFromDate", 0);
    EXTRACT_DATE("isds:ciFromDate", from_date);
    if (datecmp(configuration->from_date, from_date)) {
        code = "9999";
        message = strdup("Unexpected isds:ciFromDate value");
        error = HTTP_ERROR_CLIENT;
        goto leave;
    }

    ELEMENT_EXISTS("isds:ciTodate", 0);
    EXTRACT_DATE("isds:ciTodate", to_date);
    if (datecmp(configuration->to_date, to_date)) {
        code = "9999";
        message = strdup("Unexpected isds:ciTodate value");
        error = HTTP_ERROR_CLIENT;
        goto leave;
    }

    INSERT_LONGINTPTR(isds_response, "currentCredit",
        &configuration->current_credit);
    INSERT_STRING(isds_response, "notifEmail", configuration->email);
    if ((error = insert_ciRecords(isds_response, configuration->history))) {
        goto leave;
    }

    code = configuration->status_code;
    message = strdup(configuration->status_message);
leave:
    if (HTTP_ERROR_SERVER != error) {
        http_error next_error = insert_isds_status(isds_response, 0,
                BAD_CAST code, BAD_CAST message, NULL);
        if (HTTP_ERROR_SUCCESS != next_error) error = next_error;
    }
    free(box_id);
    free(from_date);
    free(to_date);
    free(message);
    return error;
}


/* Insert list of fulltext search results as XSD:tdbResultsArray XML tree.
 * @isds_response is XML node with the response
 * @results is list of struct server_db_result *.
 * @create_empty_root is true to create dbResults element even if @results is
 * empty. */
static http_error insert_tdbResultsArray(xmlNodePtr isds_response,
        const struct server_list *results, _Bool create_empty_root) {
    http_error error = HTTP_ERROR_SUCCESS;
    xmlNodePtr root, entry;

    if (NULL == isds_response) return HTTP_ERROR_SERVER;

    if (NULL != results || create_empty_root)
        INSERT_ELEMENT(root, isds_response, "dbResults");

    if (NULL == results) return HTTP_ERROR_SUCCESS;

    for (const struct server_list *item = results; NULL != item;
            item = item->next) {
        const struct server_db_result *result =
            (struct server_db_result *)item->data;
        
        INSERT_ELEMENT(entry, root, "dbResult");
        if (NULL == result) continue;

        INSERT_STRING(entry, "dbID", result->id);
        INSERT_STRING(entry, "dbType", result->type);
        INSERT_STRING(entry, "dbName", result->name);
        INSERT_STRING(entry, "dbAddress", result->address);
        INSERT_TMPTR(entry, "dbBiDate", result->birth_date);
        INSERT_STRING(entry, "dbICO", result->ic);
        INSERT_BOOLEANPTR(entry, "dbEffectiveOVM", &result->ovm);
        INSERT_STRING(entry, "dbSendOptions", result->send_options);
    }

leave:
    return error;
}


/* Insert list of search results as XSD:tDbOwnersArray XML tree.
 * @isds_response is XML node with the response
 * @results is list of struct server_owner_info *.
 * @create_empty_root is true to create dbResults element even if @results is
 * empty. */
static http_error insert_tDbOwnersArray(xmlNodePtr isds_response,
        const struct server_list *results, _Bool create_empty_root) {
    http_error error = HTTP_ERROR_SUCCESS;
    xmlNodePtr root, entry;

    if (NULL == isds_response) return HTTP_ERROR_SERVER;

    if (NULL != results || create_empty_root)
        INSERT_ELEMENT(root, isds_response, "dbResults");

    if (NULL == results) return HTTP_ERROR_SUCCESS;

    for (const struct server_list *item = results; NULL != item;
            item = item->next) {
        const struct server_owner_info *result =
            (struct server_owner_info *)item->data;
        
        INSERT_ELEMENT(entry, root, "dbOwnerInfo");
        if (NULL == result) continue;

        INSERT_STRING(entry, "dbID", result->dbID);
        /* Ignore aifoIsds */
        INSERT_STRING(entry, "dbType", result->dbType);
        INSERT_STRING(entry, "ic", result->ic);
        INSERT_STRING(entry, "pnFirstName", result->pnFirstName);
        INSERT_STRING(entry, "pnMiddleName", result->pnMiddleName);
        INSERT_STRING(entry, "pnLastName", result->pnLastName);
        INSERT_STRING(entry, "pnLastNameAtBirth", result->pnLastNameAtBirth);
        INSERT_STRING(entry, "firmName", result->firmName);
        INSERT_TMPTR(entry, "biDate", result->biDate);
        INSERT_STRING(entry, "biCity", result->biCity);
        INSERT_STRING(entry, "biCounty", result->biCounty);
        INSERT_STRING(entry, "biState", result->biState);
        /* Ignore adCode */
        INSERT_STRING(entry, "adCity", result->adCity);
        /* Ignore adDistrict */
        INSERT_STRING(entry, "adStreet", result->adStreet);
        INSERT_STRING(entry, "adNumberInStreet", result->adNumberInStreet);
        INSERT_STRING(entry, "adNumberInMunicipality",
                result->adNumberInMunicipality);
        INSERT_STRING(entry, "adZipCode", result->adZipCode);
        INSERT_STRING(entry, "adState", result->adState);
        INSERT_STRING(entry, "nationality", result->nationality);
        if (result->email_exists || result->email != NULL) {
            INSERT_STRING(entry, "email", result->email);
        }
        if (result->telNumber_exists || result->telNumber != NULL) {
            INSERT_STRING(entry, "telNumber", result->telNumber);
        }
        INSERT_STRING(entry, "identifier", result->identifier);
        INSERT_STRING(entry, "registryCode", result->registryCode);
        INSERT_LONGINTPTR(entry, "dbState", result->dbState);
        INSERT_BOOLEANPTR(entry, "dbEffectiveOVM", result->dbEffectiveOVM);
        INSERT_BOOLEANPTR(entry, "dbOpenAddressing", result->dbOpenAddressing);
    }

leave:
    return error;
}


/* Implement FindDataBox.
 * @arguments is pointer to struct arguments_DS_df_FindDataBox */
static http_error service_FindDataBox(
        xmlXPathContextPtr xpath_ctx,
        xmlNodePtr isds_response,
        const void *arguments) {
    http_error error = HTTP_ERROR_SUCCESS;
    const char *code = "9999";
    char *message = NULL;
    const struct arguments_DS_df_FindDataBox *configuration =
        (const struct arguments_DS_df_FindDataBox *)arguments;
    char *string = NULL;
    xmlNodePtr node;

    if (NULL == configuration || NULL == configuration->status_code ||
            NULL == configuration->status_message) {
        error = HTTP_ERROR_SERVER;
        goto leave;
    }

    /* Check request */
    error = select_element(&code, &message, xpath_ctx, "isds:dbOwnerInfo",
            &node);
    if (error) goto leave;
    xpath_ctx->node = node;

    error = element_equals_string(&code, &message, xpath_ctx,
            "isds:dbID", 1, configuration->criteria->dbID);
    if (error) goto leave;

    error = element_equals_string(&code, &message, xpath_ctx,
            "isds:dbType", 1, configuration->criteria->dbType);
    if (error) goto leave;

    error = element_equals_string(&code, &message, xpath_ctx,
            "isds:ic", 1, configuration->criteria->ic);
    if (error) goto leave;

    error = element_equals_string(&code, &message, xpath_ctx,
            "isds:pnFirstName", 1, configuration->criteria->pnFirstName);
    if (error) goto leave;

    error = element_equals_string(&code, &message, xpath_ctx,
            "isds:pnMiddleName", 1, configuration->criteria->pnMiddleName);
    if (error) goto leave;

    error = element_equals_string(&code, &message, xpath_ctx,
            "isds:pnLastName", 1, configuration->criteria->pnLastName);
    if (error) goto leave;

    error = element_equals_string(&code, &message, xpath_ctx,
            "isds:pnLastNameAtBirth", 1, configuration->criteria->pnLastNameAtBirth);
    if (error) goto leave;

    error = element_equals_string(&code, &message, xpath_ctx,
            "isds:firmName", 1, configuration->criteria->firmName);
    if (error) goto leave;

    error = element_equals_date(&code, &message, xpath_ctx,
            "isds:biDate", 1, configuration->criteria->biDate);
    if (error) goto leave;

    error = element_equals_string(&code, &message, xpath_ctx,
            "isds:biCity", 1, configuration->criteria->biCity);
    if (error) goto leave;

    error = element_equals_string(&code, &message, xpath_ctx,
            "isds:biCounty", 1, configuration->criteria->biCounty);
    if (error) goto leave;

    error = element_equals_string(&code, &message, xpath_ctx,
            "isds:biState", 1, configuration->criteria->biState);
    if (error) goto leave;

    error = element_equals_string(&code, &message, xpath_ctx,
            "isds:adCity", 1, configuration->criteria->adCity);
    if (error) goto leave;

    error = element_equals_string(&code, &message, xpath_ctx,
            "isds:adStreet", 1, configuration->criteria->adStreet);
    if (error) goto leave;

    error = element_equals_string(&code, &message, xpath_ctx,
            "isds:adNumberInStreet", 1,
            configuration->criteria->adNumberInStreet);
    if (error) goto leave;

    error = element_equals_string(&code, &message, xpath_ctx,
            "isds:adNumberInMunicipality", 1,
            configuration->criteria->adNumberInMunicipality);
    if (error) goto leave;

    error = element_equals_string(&code, &message, xpath_ctx,
            "isds:adZipCode", 1, configuration->criteria->adZipCode);
    if (error) goto leave;

    error = element_equals_string(&code, &message, xpath_ctx,
            "isds:adState", 1, configuration->criteria->adState);
    if (error) goto leave;

    error = element_equals_string(&code, &message, xpath_ctx,
            "isds:nationality", 1, configuration->criteria->nationality);
    if (error) goto leave;

    error = element_equals_string(&code, &message, xpath_ctx,
            "isds:email", 0, configuration->criteria->email);
    if (error) goto leave;

    error = element_equals_string(&code, &message, xpath_ctx,
            "isds:telNumber", 0, configuration->criteria->telNumber);
    if (error) goto leave;

    error = element_equals_string(&code, &message, xpath_ctx,
            "isds:identifier", 1, configuration->criteria->identifier);
    if (error) goto leave;

    error = element_equals_string(&code, &message, xpath_ctx,
            "isds:registryCode", 1, configuration->criteria->registryCode);
    if (error) goto leave;

    error = element_equals_integer(&code, &message, xpath_ctx,
            "isds:dbState", 1, configuration->criteria->dbState);
    if (error) goto leave;

    error = element_equals_boolean(&code, &message, xpath_ctx,
            "isds:dbEffectiveOVM", 1, configuration->criteria->dbEffectiveOVM);
    if (error) goto leave;

    error = element_equals_boolean(&code, &message, xpath_ctx,
            "isds:dbOpenAddressing", 1,
            configuration->criteria->dbOpenAddressing);
    if (error) goto leave;

    /* Build response */
    if ((error = insert_tDbOwnersArray(isds_response, configuration->results,
                    configuration->results_exists))) {
        goto leave;
    }

    code = configuration->status_code;
    message = strdup(configuration->status_message);

leave:
    if (HTTP_ERROR_SERVER != error) {
        http_error next_error = insert_isds_status(isds_response, 0,
                BAD_CAST code, BAD_CAST message, NULL);
        if (HTTP_ERROR_SUCCESS != next_error) error = next_error;
    }
    free(string);
    free(message);
    return error;
}


/* Insert list of period results as XSD:tdbPeriodsArray XML tree.
 * @isds_response is XML node with the response
 * @results is list of struct server_box_state_period *.
 * @create_empty_root is true to create Periods element even if @results is
 * empty. */
static http_error insert_tdbPeriodsArray(xmlNodePtr isds_response,
        const struct server_list *results, _Bool create_empty_root) {
    http_error error = HTTP_ERROR_SUCCESS;
    xmlNodePtr root, entry;

    if (NULL == isds_response) return HTTP_ERROR_SERVER;

    if (NULL != results || create_empty_root)
        INSERT_ELEMENT(root, isds_response, "Periods");

    if (NULL == results) return HTTP_ERROR_SUCCESS;

    for (const struct server_list *item = results; NULL != item;
            item = item->next) {
        const struct server_box_state_period *result =
            (struct server_box_state_period *)item->data;
        
        INSERT_ELEMENT(entry, root, "Period");
        if (NULL == result) continue;

        INSERT_TIMEVALPTR(entry, "PeriodFrom", result->from);
        INSERT_TIMEVALPTR(entry, "PeriodTo", result->to);
        INSERT_LONGINTPTR(entry, "DbState", &(result->dbState));
    }

leave:
    return error;
}


/* Implement GetDataBoxActivityStatus.
 * @arguments is pointer to struct arguments_DS_df_GetDataBoxActivityStatus */
static http_error service_GetDataBoxActivityStatus(
        xmlXPathContextPtr xpath_ctx,
        xmlNodePtr isds_response,
        const void *arguments) {
    http_error error = HTTP_ERROR_SUCCESS;
    const char *code = "9999";
    char *message = NULL;
    const struct arguments_DS_df_GetDataBoxActivityStatus *configuration =
        (const struct arguments_DS_df_GetDataBoxActivityStatus *)arguments;

    if (NULL == configuration || NULL == configuration->status_code ||
            NULL == configuration->status_message) {
        error = HTTP_ERROR_SERVER;
        goto leave;
    }

    /* Check request */
    error = element_equals_string(&code, &message, xpath_ctx,
            "isds:dbID", 1, configuration->box_id);
    /* ??? XML schema and textual documentation does not agree on obligatority
     * of the isds:baFrom and isds:baTo value or presence. */
    error = element_equals_time(&code, &message, xpath_ctx,
            "isds:baFrom", 1, configuration->from);
    error = element_equals_time(&code, &message, xpath_ctx,
            "isds:baTo", 1, configuration->to);
    if (error) goto leave;

    /* Build response */
    if ((error = insert_tdbPeriodsArray(isds_response, configuration->results,
                    configuration->results_exists))) {
        goto leave;
    }

    code = configuration->status_code;
    message = strdup(configuration->status_message);

leave:
    if (HTTP_ERROR_SERVER != error) {
        http_error next_error = insert_isds_status(isds_response, 0,
                BAD_CAST code, BAD_CAST message, NULL);
        if (HTTP_ERROR_SUCCESS != next_error) error = next_error;
    }
    free(message);
    return error;
}


/* Implement ISDSSearch2.
 * @arguments is pointer to struct arguments_DS_df_ISDSSearch2 */
static http_error service_ISDSSearch2(
        xmlXPathContextPtr xpath_ctx,
        xmlNodePtr isds_response,
        const void *arguments) {
    http_error error = HTTP_ERROR_SUCCESS;
    const char *code = "9999";
    char *message = NULL;
    const struct arguments_DS_df_ISDSSearch2 *configuration =
        (const struct arguments_DS_df_ISDSSearch2 *)arguments;
    char *string = NULL;

    if (NULL == configuration || NULL == configuration->status_code ||
            NULL == configuration->status_message) {
        error = HTTP_ERROR_SERVER;
        goto leave;
    }

    /* Check request */
    EXTRACT_STRING("isds:searchText", string);
    if (NULL == string) {
        message = strdup("Missing or empty isds:searchText");
        error = HTTP_ERROR_CLIENT;
        goto leave;
    }
    if (NULL != configuration->search_text &&
            xmlStrcmp(BAD_CAST configuration->search_text,
                    BAD_CAST string)) {
        code = "9999";
        message = strdup("Unexpected isds:searchText value");
        error = HTTP_ERROR_CLIENT;
        goto leave;
    }
    free(string); string = NULL;

    error = element_equals_string(&code, &message, xpath_ctx,
            "isds:searchType", 1, configuration->search_type);
    if (error) goto leave;

    error = element_equals_string(&code, &message, xpath_ctx,
            "isds:searchScope", 1, configuration->search_scope);
    if (error) goto leave;

    error = element_equals_integer(&code, &message, xpath_ctx,
            "isds:page", 1, configuration->search_page_number);
    if (error) goto leave;

    error = element_equals_integer(&code, &message, xpath_ctx,
            "isds:pageSize", 1, configuration->search_page_size);
    if (error) goto leave;

    error = element_equals_boolean(&code, &message, xpath_ctx,
            "isds:highlighting", 0, configuration->search_highlighting_value);
    if (error) goto leave;

    /* Build response */
    if (NULL != configuration->total_count)
        INSERT_ULONGINTPTR(isds_response, "totalCount",
            configuration->total_count);
    if (NULL != configuration->current_count)
        INSERT_ULONGINTPTR(isds_response, "currentCount",
            configuration->current_count);
    if (NULL != configuration->position)
        INSERT_ULONGINTPTR(isds_response, "position",
            configuration->position);
    if (NULL != configuration->last_page)
        INSERT_BOOLEANPTR(isds_response, "lastPage",
            configuration->last_page);
    if ((error = insert_tdbResultsArray(isds_response, configuration->results,
                    configuration->results_exists))) {
        goto leave;
    }

    code = configuration->status_code;
    message = strdup(configuration->status_message);

leave:
    if (HTTP_ERROR_SERVER != error) {
        http_error next_error = insert_isds_status(isds_response, 0,
                BAD_CAST code, BAD_CAST message, NULL);
        if (HTTP_ERROR_SUCCESS != next_error) error = next_error;
    }
    free(string);
    free(message);
    return error;
}


/* Common part for ChangeISDSPassword and ChangePasswordOTP.
 * @code is output pointer to static string
 * @pass_message is output pointer to auto-allocated string
 * @arguments is pointer to struct arguments_DS_DsManage_ChangeISDSPassword */
static http_error check_passwd(
        const char *username, const char *current_password,
        xmlXPathContextPtr xpath_ctx,
        char **code, char **pass_message) {
    http_error error = HTTP_ERROR_SUCCESS;
    char *message = NULL;
    char *old_password = NULL, *new_password = NULL;
    size_t length;

    if (NULL == username || NULL == current_password ||
            NULL == code || NULL == pass_message) {
        return HTTP_ERROR_SERVER;
    }

    *code = "9999";


    /* Parse request */
    EXTRACT_STRING("isds:dbOldPassword", old_password);
    if (NULL == old_password) {
        message = strdup("Empty isds:dbOldPassword");
        error = HTTP_ERROR_CLIENT;
        goto leave;
    }
    EXTRACT_STRING("isds:dbNewPassword", new_password);
    if (NULL == new_password) {
        message = strdup("Empty isds:dbOldPassword");
        error = HTTP_ERROR_CLIENT;
        goto leave;
    }

    /* Check defined cases */
    if (strcmp(current_password, old_password)) {
        *code = "1090";
        message = strdup("Bad current password");
        error = HTTP_ERROR_CLIENT;
        goto leave;
    }

    length = strlen(new_password);

    if (length < 8 || length > 32) {
        *code = "1066";
        message = strdup("Too short or too long");
        error = HTTP_ERROR_CLIENT;
        goto leave;
    }

    {
        const char lower[] = "abcdefghijklmnopqrstuvwxyz";
        const char upper[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        const char digit[] = "0123456789";
        const char special[] = "!#$%&()*+,-.:=?@[]_{}|~";
        _Bool has_lower = 0, has_upper = 0, has_digit=0;

        for (size_t i = 0; i < length; i++) {
            if (NULL != strchr(lower, new_password[i]))
                has_lower = 1;
            else if (NULL != strchr(upper, new_password[i]))
                has_upper = 1;
            else if (NULL != strchr(digit, new_password[i]))
                has_digit = 1;
            else if (NULL == strchr(special, new_password[i])) {
                *code = "1079";
                message = strdup("Password contains forbidden character");
                error = HTTP_ERROR_CLIENT;
                goto leave;
            }
        }

        if (!has_lower || !has_upper || !has_digit) {
            *code = "1080";
            message = strdup("Password does not contain lower cased letter, "
                    "upper cased letter and a digit");
            error = HTTP_ERROR_CLIENT;
            goto leave;
        }
    }

    if (!strcmp(old_password, new_password)) {
        *code = "1067";
        message = strdup("New password same as current one");
        error = HTTP_ERROR_CLIENT;
        goto leave;
    }

    if (NULL != strstr(new_password, username)) {
        *code = "1082";
        message = strdup("New password contains user ID");
        error = HTTP_ERROR_CLIENT;
        goto leave;
    }

    for (size_t i = 0; i < length - 2; i++) {
        if (new_password[i] == new_password[i+1] &&
                new_password[i] == new_password[i+2]) {
            *code = "1083";
            message = strdup("Password contains sequence "
                    "of three identical characters");
            error = HTTP_ERROR_CLIENT;
            goto leave;
        }
    }
    
    {
        const char *forbidden_prefix[] = { "qwert", "asdgf", "12345" };
        for (size_t i = 0; i < sizeof(forbidden_prefix)/sizeof(*forbidden_prefix);
                i++) {
            if (!strncmp(new_password, forbidden_prefix[i],
                        strlen(forbidden_prefix[i]))) {
                *code = "1083";
                message = strdup("Password has forbidden prefix");
                error = HTTP_ERROR_CLIENT;
                goto leave;
            }
        }
    }

    *code = "0000";
    message = strdup("Success");
leave:
    free(old_password);
    free(new_password);
    *pass_message = message;
    return error;
}


/* Implement ChangeISDSPassword.
 * @arguments is pointer to struct arguments_DS_DsManage_ChangeISDSPassword */
static http_error service_ChangeISDSPassword(
        xmlXPathContextPtr xpath_ctx,
        xmlNodePtr isds_response,
        const void *arguments) {
    http_error error = HTTP_ERROR_SUCCESS;
    char *code = "9999", *message = NULL;
    const struct arguments_DS_DsManage_ChangeISDSPassword *configuration =
        (const struct arguments_DS_DsManage_ChangeISDSPassword *)arguments;

    if (NULL == configuration || NULL == configuration->username ||
            NULL == configuration->current_password) {
        error = HTTP_ERROR_SERVER;
        goto leave;
    }

    /* Check for common password rules */
    error = check_passwd(
        configuration->username, configuration->current_password,
        xpath_ctx, &code, &message);

leave:
    if (HTTP_ERROR_SERVER != error) {
        http_error next_error = insert_isds_status(isds_response, 0,
                BAD_CAST code, BAD_CAST message, NULL);
        if (HTTP_ERROR_SUCCESS != next_error) error = next_error;
    }
    free(message);
    return error;
}


/* Implement ChangePasswordOTP.
 * @arguments is pointer to struct
 * arguments_asws_changePassword_ChangePasswordOTP */
static http_error service_ChangePasswordOTP(
        xmlXPathContextPtr xpath_ctx,
        xmlNodePtr isds_response,
        const void *arguments) {
    http_error error = HTTP_ERROR_SUCCESS;
    char *code = "9999", *message = NULL;
    const struct arguments_asws_changePassword_ChangePasswordOTP *configuration
        = (const struct arguments_asws_changePassword_ChangePasswordOTP *)
        arguments;
    char *method = NULL;

    if (NULL == configuration || NULL == configuration->username ||
            NULL == configuration->current_password) {
        error = HTTP_ERROR_SERVER;
        goto leave;
    }

    /* Chek for OTP method */
    EXTRACT_STRING("isds:dbOTPType", method);
    if (NULL == method) {
        message = strdup("Empty isds:dbOTPType");
        error = HTTP_ERROR_CLIENT;
        goto leave;
    }
    if ((configuration->method == AUTH_OTP_HMAC && strcmp(method, "HOTP")) ||
        (configuration->method == AUTH_OTP_TIME && strcmp(method, "TOTP"))) {
        message = strdup("isds:dbOTPType does not match OTP method");
        error = HTTP_ERROR_CLIENT;
        goto leave;
    }

    /* Check for common password rules */
    error = check_passwd(
        configuration->username, configuration->current_password,
        xpath_ctx, &code, &message);

leave:
    if (HTTP_ERROR_SERVER != error) {
        http_error next_error = insert_isds_status(isds_response, 0,
                BAD_CAST code, BAD_CAST message,
                BAD_CAST configuration->reference_number);
        if (HTTP_ERROR_SUCCESS != next_error) error = next_error;
    }
    free(message);
    free(method);
    return error;
}


/* Implement SendSMSCode.
 * @arguments is pointer to struct arguments_asws_changePassword_SendSMSCode */
static http_error service_SendSMSCode(
        xmlXPathContextPtr xpath_ctx,
        xmlNodePtr isds_response,
        const void *arguments) {
    const struct arguments_asws_changePassword_SendSMSCode *configuration
        = (const struct arguments_asws_changePassword_SendSMSCode *)
        arguments;
    (void)xpath_ctx;

    if (NULL == configuration || NULL == configuration->status_code ||
            NULL == configuration->status_message) {
        return HTTP_ERROR_SERVER;
    }

    return insert_isds_status(isds_response, 0,
                BAD_CAST configuration->status_code,
                BAD_CAST configuration->status_message,
                BAD_CAST configuration->reference_number);
}


/* List of implemented services */
static struct service services[] = {
    { SERVICE_DS_Dz_DummyOperation,
        "DS/dz", BAD_CAST ISDS_NS, BAD_CAST "DummyOperation",
        service_DummyOperation },
    { SERVICE_DS_Dz_ResignISDSDocument,
        "DS/dz", BAD_CAST ISDS_NS, BAD_CAST "Re-signISDSDocument",
        service_ResignISDSDocument },
    { SERVICE_DS_df_DataBoxCreditInfo,
        "DS/df", BAD_CAST ISDS_NS, BAD_CAST "DataBoxCreditInfo",
        service_DataBoxCreditInfo },
    { SERVICE_DS_df_FindDataBox,
        "DS/df", BAD_CAST ISDS_NS, BAD_CAST "FindDataBox",
        service_FindDataBox },
    { SERVICE_DS_df_GetDataBoxActivityStatus,
        "DS/df", BAD_CAST ISDS_NS, BAD_CAST "GetDataBoxActivityStatus",
        service_GetDataBoxActivityStatus },
    { SERVICE_DS_df_ISDSSearch2,
        "DS/df", BAD_CAST ISDS_NS, BAD_CAST "ISDSSearch2",
        service_ISDSSearch2 },
    { SERVICE_DS_DsManage_ChangeISDSPassword,
        "DS/DsManage", BAD_CAST ISDS_NS, BAD_CAST "ChangeISDSPassword",
        service_ChangeISDSPassword },
    { SERVICE_DS_Dx_EraseMessage,
        "DS/dx", BAD_CAST ISDS_NS, BAD_CAST "EraseMessage",
        service_EraseMessage },
    { SERVICE_asws_changePassword_ChangePasswordOTP,
        "/asws/changePassword", BAD_CAST OISDS_NS, BAD_CAST "ChangePasswordOTP",
        service_ChangePasswordOTP },
    { SERVICE_asws_changePassword_SendSMSCode,
        "/asws/changePassword", BAD_CAST OISDS_NS, BAD_CAST "SendSMSCode",
        service_SendSMSCode },
};


/* Makes known all relevant namespaces to given XPath context
 * @xpath_ctx is XPath context
 * @otp_ns selects name space for the request and response know as "isds".
 * Use true for OTP-authenticated password change services, otherwise false.
 * @message_ns selects proper message name space. Unsigned and signed
 * messages and delivery info's differ in prefix and URI.
 * @return 0 in success, otherwise not 0. */
static int register_namespaces(xmlXPathContextPtr xpath_ctx,
        const _Bool otp_ns, const message_ns_type message_ns) {
    const xmlChar *service_namespace = NULL;
    const xmlChar *message_namespace = NULL;

    if (!xpath_ctx) return -1;

    if (otp_ns) {
        service_namespace = BAD_CAST OISDS_NS;
    } else {
        service_namespace = BAD_CAST ISDS_NS;
    }

    switch(message_ns) {
        case MESSAGE_NS_1:
            message_namespace = BAD_CAST ISDS1_NS; break;
        case MESSAGE_NS_UNSIGNED:
            message_namespace = BAD_CAST ISDS_NS; break;
        case MESSAGE_NS_SIGNED_INCOMING:
            message_namespace = BAD_CAST SISDS_INCOMING_NS; break;
        case MESSAGE_NS_SIGNED_OUTGOING:
            message_namespace = BAD_CAST SISDS_OUTGOING_NS; break;
        case MESSAGE_NS_SIGNED_DELIVERY:
            message_namespace = BAD_CAST SISDS_DELIVERY_NS; break;
        default:
            return -1;
    }

    if (xmlXPathRegisterNs(xpath_ctx, BAD_CAST "soap", BAD_CAST SOAP_NS))
        return -1;
    if (xmlXPathRegisterNs(xpath_ctx, BAD_CAST "isds", service_namespace))
        return -1;
    if (xmlXPathRegisterNs(xpath_ctx, BAD_CAST "sisds", message_namespace))
        return -1;
    if (xmlXPathRegisterNs(xpath_ctx, BAD_CAST "xs", BAD_CAST SCHEMA_NS))
        return -1;
    if (xmlXPathRegisterNs(xpath_ctx, BAD_CAST "deposit", BAD_CAST DEPOSIT_NS))
        return -1;
    return 0;
}


/* Parse soap request, pass it to service endpoint and respond to it.
 * It sends final HTTP response. */
void soap(const struct http_connection *connection,
        const struct service_configuration *configuration,
        const void *request, size_t request_length, const char *end_point) {
    xmlDocPtr request_doc = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr request_soap_body = NULL;
    xmlNodePtr isds_request = NULL; /* pointer only */
    _Bool service_handled = 0, service_passed = 0;
    xmlDocPtr response_doc = NULL;
    xmlNodePtr response_soap_envelope = NULL, response_soap_body = NULL,
               isds_response = NULL;
    xmlNsPtr soap_ns = NULL, isds_ns = NULL;
    char *response_name = NULL;
    xmlBufferPtr http_response_body = NULL;
    xmlSaveCtxtPtr save_ctx = NULL;


    if (NULL == configuration) {
        http_send_response_500(connection,
                "Second argument of soap() is NULL");
        return;
    }

    if (NULL == request || request_length == 0) {
        http_send_response_400(connection, "Client sent empty body");
        return;
    }

    request_doc = xmlParseMemory(request, request_length);
    if (NULL == request_doc) {
        http_send_response_400(connection, "Client sent invalid XML document");
        return;
    }
    
    xpath_ctx = xmlXPathNewContext(request_doc);
    if (NULL == xpath_ctx) {
        xmlFreeDoc(request_doc);
        http_send_response_500(connection, "Could not create XPath context");
        return;
    }

    if (register_namespaces(xpath_ctx, 0, MESSAGE_NS_UNSIGNED)) {
        xmlXPathFreeContext(xpath_ctx);
        xmlFreeDoc(request_doc);
        http_send_response_500(connection,
                "Could not register name spaces to the XPath context");
        return;
    }

    /* Get SOAP Body */
    request_soap_body = xmlXPathEvalExpression(
            BAD_CAST "/soap:Envelope/soap:Body", xpath_ctx);
    if (NULL == request_soap_body) {
        xmlXPathFreeContext(xpath_ctx);
        xmlFreeDoc(request_doc);
        http_send_response_400(connection, "Client sent invalid SOAP request");
        return;
    }
    if (xmlXPathNodeSetIsEmpty(request_soap_body->nodesetval)) {
        xmlXPathFreeObject(request_soap_body);
        xmlXPathFreeContext(xpath_ctx);
        xmlFreeDoc(request_doc);
        http_send_response_400(connection,
                "SOAP request does not contain SOAP Body element");
        return;
    }
    if (request_soap_body->nodesetval->nodeNr > 1) {
        xmlXPathFreeObject(request_soap_body);
        xmlXPathFreeContext(xpath_ctx);
        xmlFreeDoc(request_doc);
        http_send_response_400(connection,
                "SOAP response has more than one Body element");
        return;
    }
    isds_request = request_soap_body->nodesetval->nodeTab[0]->children;
    if (isds_request->next != NULL) {
        xmlXPathFreeObject(request_soap_body);
        xmlXPathFreeContext(xpath_ctx);
        xmlFreeDoc(request_doc);
        http_send_response_400(connection, "SOAP body has more than one child");
        return;
    }
    if (isds_request->type != XML_ELEMENT_NODE || isds_request->ns == NULL ||
            NULL == isds_request->ns->href) {
        xmlXPathFreeObject(request_soap_body);
        xmlXPathFreeContext(xpath_ctx);
        xmlFreeDoc(request_doc);
        http_send_response_400(connection,
                "SOAP body does not contain a name-space-qualified element");
        return;
    }

    /* Build SOAP response envelope */
    response_doc = xmlNewDoc(BAD_CAST "1.0");
    if (!response_doc) {
        http_send_response_500(connection,
                "Could not build SOAP response document");
        goto leave;
    }
    response_soap_envelope = xmlNewNode(NULL, BAD_CAST "Envelope");
    if (!response_soap_envelope) {
        http_send_response_500(connection,
                "Could not build SOAP response envelope");
        goto leave;
    }
    xmlDocSetRootElement(response_doc, response_soap_envelope);
    /* Only this way we get namespace definition as @xmlns:soap,
     * otherwise we get namespace prefix without definition */
    soap_ns = xmlNewNs(response_soap_envelope, BAD_CAST SOAP_NS, NULL);
    if(NULL == soap_ns) {
        http_send_response_500(connection, "Could not create SOAP name space");
        goto leave;
    }
    xmlSetNs(response_soap_envelope, soap_ns);
    response_soap_body = xmlNewChild(response_soap_envelope, NULL,
            BAD_CAST "Body", NULL);
    if (!response_soap_body) {
        http_send_response_500(connection,
                "Could not add Body to SOAP response envelope");
        goto leave;
    }
    /* Append ISDS response element */
    if (-1 == test_asprintf(&response_name, "%s%s", isds_request->name,
                "Response")) {
        http_send_response_500(connection,
                "Could not buld ISDS resposne element name");
        goto leave;
    }
    isds_response = xmlNewChild(response_soap_body, NULL,
            BAD_CAST response_name, NULL);
    free(response_name);
    if (NULL == isds_response) {
        http_send_response_500(connection,
                "Could not add ISDS response element to SOAP response body");
        goto leave;
    }
    isds_ns = xmlNewNs(isds_response, isds_request->ns->href, NULL);
    if(NULL == isds_ns) {
        http_send_response_500(connection,
                "Could not create a name space for the response body");
        goto leave;
    }
    xmlSetNs(isds_response, isds_ns);

    /* Dispatch request to service */
    for (size_t i = 0; i < sizeof(services)/sizeof(services[0]); i++) {
        if (!strcmp(services[i].end_point, end_point) &&
                !xmlStrcmp(services[i].name_space, isds_request->ns->href) &&
                !xmlStrcmp(services[i].name, isds_request->name)) {
            /* Check if the configuration is enabled and find configuration */
            for (const struct service_configuration *service = configuration;
                    service->name != SERVICE_END; service++) {
                if (service->name == services[i].id) {
                    service_handled = 1;
                    if (!xmlStrcmp(services[i].name_space, BAD_CAST OISDS_NS)) {
                        /* Alias "isds" XPath identifier to OISDS_NS */
                        if (register_namespaces(xpath_ctx, 1,
                                    MESSAGE_NS_UNSIGNED)) {
                            http_send_response_500(connection,
                                    "Could not register name spaces to the "
                                    "XPath context");
                            break;
                        }
                    }
                    xpath_ctx->node = isds_request;
                    if (HTTP_ERROR_SERVER != services[i].function(
                                xpath_ctx,
                                isds_response,
                                service->arguments)) {
                        service_passed = 1;
                    } else {
                        http_send_response_500(connection,
                                "Internal server error while processing "
                                "ISDS request");
                    }
                }
            }
            break;
        }
    }
    
    /* Send response */
    if (service_passed) {
        /* Serialize the SOAP response */
        http_response_body = xmlBufferCreate();
        if (NULL == http_response_body) {
            http_send_response_500(connection,
                    "Could not create xmlBuffer for response serialization");
            goto leave;
        }
        /* Last argument 1 means format the XML tree. This is pretty but it breaks
         * XML document transport as it adds text nodes (indentiation) between
         * elements. */
        save_ctx = xmlSaveToBuffer(http_response_body, "UTF-8", 0);
        if (NULL == save_ctx) {
            http_send_response_500(connection, "Could not create XML serializer");
            goto leave;
        }
        /* XXX: According LibXML documentation, this function does not return
         * meaningful value yet */
        xmlSaveDoc(save_ctx, response_doc);
        if (-1 == xmlSaveFlush(save_ctx)) {
            http_send_response_500(connection,
                    "Could not serialize SOAP response");
            goto leave;
        }

        http_send_response_200(connection, http_response_body->content,
                http_response_body->use, soap_mime_type);
    }

leave:
    xmlSaveClose(save_ctx);
    xmlBufferFree(http_response_body);

    xmlFreeDoc(response_doc);

    xmlXPathFreeObject(request_soap_body);
    xmlXPathFreeContext(xpath_ctx);
    xmlFreeDoc(request_doc);

    if (!service_handled) {
        http_send_response_500(connection,
                "Requested ISDS service not implemented");
    }

}


