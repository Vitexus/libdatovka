#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "isds.h"

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


/* Initialize ISDS library.
 * Global function, must be called before other functions. */
isds_error isds_init(void) {
    if (curl_global_init(CURL_GLOBAL_ALL)) {
        return IE_ERROR;
    }
    return IE_SUCCESS;
}

/* Deinicialize ISDS library.
 * Global function, must be called as last library function. */
isds_error isds_cleanup(void) {
    curl_global_cleanup();
    return IE_SUCCESS;
}


/* Return text description of ISDS error */
char *isds_strerror(const isds_error error) {
    switch (error) {
        case IE_SUCCESS:
            return(_("Success")); break;
        case IE_ERROR:
            return(_("Unspecified error")); break;
        case IE_NOTSUP:
            return(_("Not supported")); break;
        case IE_INVAL:
            return(_("Invalid value")); break;
        case IE_INVALID_CONTEXT:
            return(_("Invalid context")); break;
        case IE_NOT_LOGGED_IN:
            return(_("Not logged in")); break;
        case IE_CONNECTION_CLOSED:
            return(_("Connection closed")); break;
        case IE_TIMED_OUT:
            return(_("Timed out")); break;
        case IE_NOEXIST:
            return(_("Not exist")); break;
        case IE_NOMEM:
            return(_("Out of memmory")); break;
        case IE_NETWORK:
            return(_("Network problem")); break;
        default:
            return(_("Unknown error"));
    }
}


/* Create ISDS context.
 * Each context can be used for different sessions to (possibly) differnet
 * ISDS server with different credentials. */
struct isds_ctx *isds_ctx_create(void) {
    struct isds_ctx *context;
    context = malloc(sizeof(struct isds_ctx));
    memset(context, 0, sizeof(*context));
    return context;
}


/* Destroy ISDS context and free memmory.
 * @context will be NULLed on success. */
isds_error isds_ctx_free(struct isds_ctx **context) {
    if (!context || !*context) {
        return IE_ERROR;
    }
    free(*context);
    *context = NULL;
    return IE_SUCCESS;
}


/* Return long message text produced by library fucntion, e.g. detailed error
 * mesage. Returned pointer is only valid until new library function is
 * called for the same context. Could be NULL, especially if NULL context is
 * supplied. */
char *isds_long_message(const struct isds_ctx *context) {
    if (!context) return NULL;
    return context->long_message;
}


/* Stores message into context' long_message buffer.
 * Application can pick the message up using isds_long_message(). */
isds_error isds_log_message(struct isds_ctx *context, const char *message) {
    char *buffer;
    size_t length;
    
    if (!context) return IE_INVALID_CONTEXT;
    if (!message) return IE_INVAL;
    
    /* FIXME: Check for integer overflow */
    length = 1 + strlen(message);
    buffer = realloc(context->long_message, length);
    if (!buffer) return IE_NOMEM;

    strncpy(buffer, message, length);

    context->long_message = buffer;
    return IE_SUCCESS;
}


/* Connect to given url.
 * It just makes TCP connection to ISDS server found in @url hostname part. */
/*int isds_connect(struct isds_ctx *context, const char *url);*/

/* Set timeout in miliseconds for each network job like connecting to server
 * or sending message. Use 0 to disable timeout limits. */
isds_error isds_set_timeout(struct isds_ctx *context, const unsigned int timeout) {
    return IE_NOTSUP;
}


/* Connect and log in into ISDS server.
 * @url is address of ISDS web service
 * @username is user name of ISDS user
 * @password is user's secret password
 * @certificate is NULL terminated string with PEM formated client's
 * certificate. Use NULL if only password autentication should be performed.
 * @key is private key for client's certificate as (base64 encoded?) NULL
 * terminated string. Use NULL if only password autentication is desired.
 * */
isds_error isds_login(struct isds_ctx *context, const char *url, const char *username,
        const char *password, const char *certificate, const char* key) {
    CURLcode curl_err;

    if (!context) return IE_INVALID_CONTEXT;
    if (!url || !username || !password) return IE_INVAL;
    if (certificate || key) return IE_NOTSUP;

    context->curl = curl_easy_init();
    if (!(context->curl))
        return IE_NETWORK;

    curl_err = curl_easy_setopt(context->curl, CURLOPT_URL, url);
    if (curl_err) {
        isds_log_message(context, curl_easy_strerror(curl_err));
        curl_easy_cleanup(context->curl);
        context->curl = NULL;
        return IE_NETWORK;
    }

    curl_err = curl_easy_perform(context->curl);
    if (curl_err) {
        isds_log_message(context, curl_easy_strerror(curl_err));
        curl_easy_cleanup(context->curl);
        context->curl = NULL;
        return IE_NETWORK;
    }


    return IE_NOTSUP;
}


/* Log out from ISDS server and close connection. */
isds_error isds_logout(struct isds_ctx *context) {
    if (!context) return IE_INVALID_CONTEXT;

    if (context->curl) {
        curl_easy_cleanup(context->curl);
    }

    if (!(context->cookie)) return IE_NOT_LOGGED_IN;
    return IE_NOTSUP;
}


/*int isds_get_message(struct isds_ctx *context, const unsigned int id,
        struct isds_message **message);
int isds_send_message(struct isds_ctx *context, struct isds_message *message);
int isds_list_messages(struct isds_ctx *context, struct isds_message **message);
int isds_find_recipient(struct isds_ctx *context, const struct address *pattern,
        struct isds_address **address);

int isds_message_free(struct isds_message **message);
int isds_address_free(struct isds_address **address);
*/
