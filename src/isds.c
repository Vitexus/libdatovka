#define _XOPEN_SOURCE 500   /* strdup from string.h */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "isds_priv.h"
#include "utils.h"
#include "soap.h"


/* Initialize ISDS library.
 * Global function, must be called before other functions.
 * If it failes you can not use ISDS library and must call isds_cleanup() to
 * free partially inititialized global variables. */
isds_error isds_init(void) {
    /* NULL global variables */
    xml_node = NULL;
    soap_ns = NULL;
    log_facilities = ILF_NONE;
    log_level = ILL_NONE;

    /* Initialize CURL */
    if (curl_global_init(CURL_GLOBAL_ALL)) {
        return IE_ERROR;
    }

    /* This can _exit() current program. Find not so assertive check. */
    LIBXML_TEST_VERSION;

    /* Allocate global variables */
    if (!(xml_node = xmlNewNode(NULL, BAD_CAST "global-element")))
        return IE_ERROR;
    if (!(soap_ns = xmlNewNs(NULL, BAD_CAST SOAP_NS, BAD_CAST "soap")))
        return IE_ERROR;
    if (!(isds_ns = xmlNewNs(NULL, BAD_CAST ISDS_NS, BAD_CAST "isds")))
        return IE_ERROR;

    return IE_SUCCESS;
}


/* Deinicialize ISDS library.
 * Global function, must be called as last library function. */
isds_error isds_cleanup(void) {
    /* XML */
    xmlFreeNs(isds_ns);
    xmlFreeNs(soap_ns);
    xmlFreeNode(xml_node);
    xmlCleanupParser();
    
    /* Curl */
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
            return(_("Out of memory")); break;
        case IE_NETWORK:
            return(_("Network problem")); break;
        case IE_SOAP:
            return(_("SOAP problem")); break;
        case IE_XML:
            return(_("XML problem")); break;
        default:
            return(_("Unknown error"));
    }
}


/* Create ISDS context.
 * Each context can be used for different sessions to (possibly) differnet
 * ISDS server with different credentials. */
struct isds_ctx *isds_ctx_create(void) {
    struct isds_ctx *context;
    context = malloc(sizeof(*context));
    if (context) memset(context, 0, sizeof(*context));
    return context;
};


/* Destroy ISDS context and free memmory.
 * @context will be NULLed on success. */
isds_error isds_ctx_free(struct isds_ctx **context) {
    if (!context || !*context) {
        return IE_INVALID_CONTEXT;
    }
  
    /* Discard credentials */
    isds_logout(*context);

    /* Free other structures */
    free((*context)->long_message);

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
 * Application can pick the message up using isds_long_message().
 * NULL @message truncates the buffer but does not deallocate it. */
_hidden isds_error isds_log_message(struct isds_ctx *context,
        const char *message) {
    char *buffer;
    size_t length;
    
    if (!context) return IE_INVALID_CONTEXT;
    
    /* FIXME: Check for integer overflow */
    length = 1 + ((message) ? strlen(message) : 0);
    buffer = realloc(context->long_message, length);
    if (!buffer) return IE_NOMEM;

    if (message)
        strcpy(buffer, message);
    else
        *buffer = '\0';

    context->long_message = buffer;
    return IE_SUCCESS;
}


/* Appends message into context' long_message buffer.
 * Application can pick the message up using isds_long_message().
 * NULL message has void effect. */
_hidden isds_error isds_append_message(struct isds_ctx *context,
        const char *message) {
    char *buffer;
    size_t old_length, length;
    
    if (!context) return IE_INVALID_CONTEXT;
    if (!message) return IE_SUCCESS;
    if (!context->long_message)
        return isds_log_message(context, message);
    
    old_length = strlen(context->long_message);
    /* FIXME: Check for integer overflow */
    length = 1 + old_length + strlen(message);
    buffer = realloc(context->long_message, length);
    if (!buffer) return IE_NOMEM;

    strcpy(buffer + old_length, message);

    context->long_message = buffer;
    return IE_SUCCESS;
}


/* Set logging up.
 * @facilities is bitmask of isds_log_facility values,
 * @level is verbosity level. */
void isds_set_logging(const unsigned int facilities,
        const isds_log_level level) {
    log_facilities = facilities;
    log_level = level;
}


/* Log @message in class @facility with log @level into global log. @message
 * is printf(3) formating string, variadic arguments may be neccessary.
 * For debugging purposes. */
_hidden isds_error isds_log(const isds_log_facility facility,
        const isds_log_level level, const char *message, ...) {
    va_list ap;

    if (level > log_level) return IE_SUCCESS;
    if (!(log_facilities & facility)) return IE_SUCCESS;
    if (!message) return IE_INVAL;

    /* TODO: Allow to register output function privided by application
     * (e.g. fprintf to stderr or copy to text area GUI widget). */

    va_start(ap, message);
    vfprintf(stderr, message, ap);
    va_end(ap);
    /* Line buffered printf is default.
     * fflush(stderr);*/

    return IE_SUCCESS;
}


/* Connect to given url.
 * It just makes TCP connection to ISDS server found in @url hostname part. */
/*int isds_connect(struct isds_ctx *context, const char *url);*/

/* Set timeout in miliseconds for each network job like connecting to server
 * or sending message. Use 0 to disable timeout limits. */
isds_error isds_set_timeout(struct isds_ctx *context,
        const unsigned int timeout) {
    if (!context) return IE_INVALID_CONTEXT;

    context->timeout = timeout;

    if (context->curl) {
        CURLcode curl_err;

        curl_err = curl_easy_setopt(context->curl, CURLOPT_NOSIGNAL, 1);
        if (!curl_err) 
            curl_err = curl_easy_setopt(context->curl, CURLOPT_TIMEOUT_MS,
                    context->timeout);
        if (curl_err) return IE_ERROR;
    }

    return IE_SUCCESS;
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
    isds_error err = IE_NOT_LOGGED_IN;
    isds_error soap_err;
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr request = NULL;
    xmlNodePtr response = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!url || !username || !password) return IE_INVAL;
    if (certificate || key) return IE_NOTSUP;

    /* Store configuration */
    free(context->url);
    context->url = strdup(url);
    if (!(context->url))
        return IE_NOMEM;

    free(context->username);
    context->username = strdup(username);
    if (!(context->username))
        return IE_NOMEM;

    /* FIXME: mlock password
     * (I have a library) */
    free(context->password);
    context->password = strdup(password);
    if (!(context->password))
        return IE_NOMEM;

    context->curl = curl_easy_init();
    if (!(context->curl))
        return IE_ERROR;

    /* Build login request */
    request = xmlNewNode(NULL, BAD_CAST "DummyOperation");
    if (!request) {
        isds_log_message(context, _("Could build ISDS login request"));
        return IE_ERROR;
    }
    isds_ns = xmlNewNs(request, BAD_CAST ISDS_NS, NULL);
    if(!isds_ns) {
        isds_log_message(context, _("Could not create ISDS name space"));
        xmlFreeNode(request);
        return IE_ERROR;
    }
    xmlSetNs(request, isds_ns);

    soap_err = soap(context, "dz", request, &response);
   
    /* Destroy login request */
    xmlFreeNode(request);

    if (soap_err) {
        xmlFreeNodeList(response);
        curl_easy_cleanup(context->curl);
        context->curl = NULL;
        return soap_err;
    }

    /* XXX: Untill we don't propagate HTTP code 500 or 4xx, we can be sure
     * authentication succeeded if soap_err == IE_SUCCESS */
    err = IE_SUCCESS;
    /* XXX: Dummy cookie.
     * Probably, we could remove the cookie from context because CURL
     * administrer it on its own and soap()/http() does autologin now. */
    free(context->cookie);
    context->cookie = strdup("42");
    if (!context->cookie) {
        err = IE_NOMEM;
    }

    xmlFreeNodeList(response);

    return err;
}


/* Log out from ISDS server discards credentials and connection configuration. */
isds_error isds_logout(struct isds_ctx *context) {
    if (!context) return IE_INVALID_CONTEXT;

    /* Close connection */
    if (context->curl) {
        curl_easy_cleanup(context->curl);
        context->curl = NULL;
    }

    /* Discard credentials */
    free(context->url);
    context->url = NULL;
    free(context->username);
    context->username = NULL;
    free(context->password);
    context->password = NULL;

    if (!context->cookie) 
        return IE_NOT_LOGGED_IN;
    free(context->cookie);
    context->cookie = NULL;

    return IE_SUCCESS;
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

/* Makes known all relevant namespaces to give @xpat_ctx */
_hidden isds_error register_namespaces(xmlXPathContextPtr xpath_ctx) {
    if (!xpath_ctx) return IE_ERROR;

    if (xmlXPathRegisterNs(xpath_ctx, BAD_CAST "soap", BAD_CAST SOAP_NS))
        return IE_ERROR;
    if (xmlXPathRegisterNs(xpath_ctx, BAD_CAST "isds", BAD_CAST ISDS_NS))
        return IE_ERROR;
    if (xmlXPathRegisterNs(xpath_ctx, BAD_CAST "xs", BAD_CAST SCHEMA_NS))
        return IE_ERROR;
    return IE_SUCCESS;
}

