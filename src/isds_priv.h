#ifndef __ISDS_ISDS_PRIV_H__
#define __ISDS_ISDS_PRIV_H__

/* Feature macros to enable some declarations. This is kept here to align all
 * header files to one shape. */
#ifndef _XOPEN_SOURCE
/* >= 500: strdup(3) from string.h, strptime(3) from time.h */
/* >= 600: setenv(3) */
/* >= 700: strndup(3) from string.h */
#define _XOPEN_SOURCE 700
#endif

#ifndef _POSIX_SOURCE
/* defined: strtok_r */
#define _POSIX_SOURCE   
#endif

/* Structures not to export outside library */
#include "../config.h"
#include "isds.h"
#if HAVE_LIBCURL
    #include <curl/curl.h>
#endif
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/xmlsave.h>
#include "gettext.h"

#define _(x) ((const char *) dgettext(PACKAGE, (x)))
#define N_(x) (x)

#define SOAP_NS "http://schemas.xmlsoap.org/soap/envelope/"
#define SOAP2_NS "http://www.w3.org/2003/05/soap-envelope"
#define ISDS1_NS "http://isds.czechpoint.cz"
#define ISDS_NS "http://isds.czechpoint.cz/v20"
#define SISDS_INCOMING_NS "http://isds.czechpoint.cz/v20/message"
#define SISDS_OUTGOING_NS "http://isds.czechpoint.cz/v20/SentMessage"
#define SISDS_DELIVERY_NS "http://isds.czechpoint.cz/v20/delivery"
#define SCHEMA_NS "http://www.w3.org/2001/XMLSchema"
#define DEPOSIT_NS "urn:uschovnaWSDL"


/* Used to choose proper name space for message elements.
 * See _isds_register_namespaces(). */
typedef enum {
    MESSAGE_NS_1,
    MESSAGE_NS_UNSIGNED,
    MESSAGE_NS_SIGNED_INCOMING,
    MESSAGE_NS_SIGNED_OUTGOING,
    MESSAGE_NS_SIGNED_DELIVERY
} message_ns_type;

/* Type of a context */
typedef enum {
    CTX_TYPE_NONE = 0,  /* Not configured for any connection yet */
    CTX_TYPE_ISDS,      /* Connection to ISDS */
    CTX_TYPE_CZP,       /* Connection to Czech POINT document deposit */
    CTX_TYPE_TESTING_REQUEST_COLLECTOR /* Connection to server collectiong
                                          new testing box requests */
} context_type;

/* Global variables.
 * Allocated in isds_init() and deallocated in isds_cleanup(). */
unsigned int log_facilities;
isds_log_level log_level;
isds_log_callback log_callback;     /* Pass global log message to application.
                                       NULL to log to stderr itself */
void *log_callback_data;            /* Application specific data to pass to
                                       registered log_callback function */
const char *version_gpgme;          /* Static string with GPGME version */
const char *version_gcrypt;         /* Static string with gcrypt version */
const char *version_expat;          /* Static string with expat version */

/* End of global variables */

/* Context */
struct isds_ctx {
    context_type type;      /* Context type */
#if HAVE_LIBCURL
    unsigned int timeout;   /* milliseconds */
    char *url;              /* URL of the ISDS web service */
    char *username;
    char *password;
    struct isds_pki_credentials *pki_credentials;
    struct isds_otp *otp;   /* Pointer (no copy) to OTP credentials */
    CURL *curl;             /* CURL session handle */
    _Bool *tls_verify_server;   /* Verify the server? */
    isds_progress_callback progress_callback;  /* Call it during
                                                   communication with server.
                                                   NULL for nothing */
    void *progress_callback_data;       /* Application provided argument
                                           for progress_callback */
    char *tls_ca_file;      /* File name with CA certificates */
    char *tls_ca_dir;       /* Directory name with CA certificates */
    char *tls_crl_file;     /* File name with CRL in PEM format */
#endif /* HAVE_LIBCURL */
    _Bool normalize_mime_type; /* Normalize document MIME types? */
    char *long_message;     /* message buffer */
};

/* Stores message into context' long_message buffer.
 * Application can pick the message up using isds_long_message().
 * NULL @message truncates the buffer but does not deallocate it.
 * @message is coded in locale encoding */
isds_error isds_log_message(struct isds_ctx *context, const char *message);

/* Appends message into context' long_message buffer.
 * Application can pick the message up using isds_long_message().
 * NULL message has void effect. */
isds_error isds_append_message(struct isds_ctx *context, const char *message);

/* Stores formated message into context' long_message buffer.
 * Application can pick the message up using isds_long_message(). */
isds_error isds_printf_message(struct isds_ctx *context,
        const char *format, ...);

/* Log @message in class @facility with log @level into global log. @message
 * is printf(3) formating string, variadic arguments may be neccessary.
 * For debugging purposes. */
isds_error isds_log(const isds_log_facility facility,
        const isds_log_level level, const char *message, ...);

/* Makes known all relevant namespaces to given XPath context
 * @xpath_ctx is XPath context
 * @message_ns selects propper message name space. Unsisnged and signed
 * messages and delivery infos differ in prefix and URI. */
isds_error _isds_register_namespaces(xmlXPathContextPtr xpath_ctx,
        const message_ns_type message_ns);

#if HAVE_LIBCURL
/* Discard credentials.
 * Only that. It does not cause log out, connection close or similar. */
isds_error _isds_discard_credentials(struct isds_ctx *context);
#endif /* HAVE_LIBCURL */

#endif
