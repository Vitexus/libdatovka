#define _XOPEN_SOURCE 500   /* strdup from string.h */
#include <stdlib.h>
#include <string.h>
#include "isds_priv.h"
#include "utils.h"
#include "soap.h"


/* Initialize ISDS library.
 * Global function, must be called before other functions. */
isds_error isds_init(void) {
    if (curl_global_init(CURL_GLOBAL_ALL)) {
        return IE_ERROR;
    }

    /* This can _exit() current program. Find not so assertive check. */
    LIBXML_TEST_VERSION

    return IE_SUCCESS;
}


/* Deinicialize ISDS library.
 * Global function, must be called as last library function. */
isds_error isds_cleanup(void) {
    curl_global_cleanup();
    xmlCleanupParser();

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
    context = malloc(sizeof(struct isds_ctx));
    memset(context, 0, sizeof(*context));
    return context;
};


/* Destroy ISDS context and free memmory.
 * @context will be NULLed on success. */
isds_error isds_ctx_free(struct isds_ctx **context) {
    if (!context || !*context) {
        return IE_ERROR;
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
    length = 1 + (message) ? strlen(message) : 0;
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
    isds_error err = IE_NOT_LOGGED_IN;
    isds_error soap_err;
    char request[] =  "<DummyOperation/>";
    size_t request_length = sizeof(request);
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

    soap_err = soap(context, "login", request, request_length, &response);
    
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

    if (xmlXPathRegisterNs(xpath_ctx, BAD_CAST "soap",
                BAD_CAST "http://www.w3.org/2003/05/soap-envelope"))
        return IE_ERROR;
    if (xmlXPathRegisterNs(xpath_ctx, BAD_CAST "isds",
                BAD_CAST "http://isds.czechpoint.cz/v20"))
        return IE_ERROR;
    if (xmlXPathRegisterNs(xpath_ctx, BAD_CAST "xs",
                BAD_CAST "http://www.w3.org/2001/XMLSchema"))
        return IE_ERROR;
    return IE_SUCCESS;
}

