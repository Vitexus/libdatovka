#ifndef __ISDS_ISDS_PRIV_H__
#define __ISDS_ISDS_PRIV_H__

/* Structures not to export outside library */

#include "isds.h"
#include <curl/curl.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#define _(x) (x)

struct isds_ctx {
    unsigned int timeout;   /* milliseconds */
    char *url;              /* URL of the ISDS web service */
    char *username;
    char *password;
    char *client_certificate;
    char *private_key;
    char *cookie;           /* Autorization token for ISDS HTTP session */
    CURL *curl;             /* CURL session handle */
    char *long_message;     /* message buffer */
};

/* Stores message into context' long_message buffer.
 * Application can pick the message up using isds_long_message(). */
isds_error isds_log_message(struct isds_ctx *context, const char *message);

/* Appends message into context' long_message buffer.
 * Application can pick the message up using isds_long_message().
 * NULL message has void effect. */
isds_error isds_append_message(struct isds_ctx *context, const char *message);

#endif
