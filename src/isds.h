#ifndef __ISDS_ISDS_H__
#define __ISDS_ISDS_H__

/* Public interface for libisds.
 * Private declarations in isds_priv.h. */

struct isds_ctx;    /* Context for specific ISDS box */

typedef enum {
    IE_SUCCESS = 0, /* No error, just for C conveniece (0 means Ok) */
    IE_ERROR,       /* unsepcified error */
    IE_NOTSUP,
    IE_INVAL,
    IE_INVALID_CONTEXT,
    IE_NOT_LOGGED_IN,
    IE_CONNECTION_CLOSED,
    IE_TIMED_OUT,
    IE_NOEXIST,
    IE_NOMEM,
    IE_NETWORK,
    IE_SOAP
} isds_error;

/* Return text description of ISDS error */
char *isds_strerror(const isds_error error);

struct isds_address {
    struct isds_address *next;
    char *box_id;
    char *name;
};

struct isds_message {
    struct isds_message *next;
    struct isds_address *sender;
    struct isds_address *recipient;
    char *subject;
};


/* Initialize ISDS library.
 * Global function, must be called before other functions. */
isds_error isds_init(void);

/* Deinicialize ISDS library.
 * Global function, must be called as last library function. */
isds_error isds_cleanup(void);

/* Create ISDS context.
 * Each context can be used for different sessions to (possibly) different
 * ISDS server with different credentials.
 * Returns new context, or NULL */
struct isds_ctx *isds_ctx_create(void);

/* Destroy ISDS context and free memmory.
 * @context will be NULLed on success. */
isds_error isds_ctx_free(struct isds_ctx **context);

/* Return long message text produced by library fucntion, e.g. detailed error
 * mesage. Returned pointer is only valid until new library function is
 * called. */
char *isds_long_message(const struct isds_ctx *context);

/* Connect to given url.
 * It just makes TCP connection to ISDS server found in @url hostname part. */
/*int isds_connect(struct isds_ctx *context, const char *url);*/

/* Set timeout in miliseconds for each network job like connecting to server
 * or sending message. Use 0 to disable timeout limits. */
isds_error isds_set_timeout(struct isds_ctx *context, const unsigned int timeout);

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
        const char *password, const char *certificate, const char* key);

/* Log out from ISDS server and close connection. */
isds_error isds_logout(struct isds_ctx *context);


/*int isds_get_message(struct isds_ctx *context, const unsigned int id,
        struct isds_message **message);
int isds_send_message(struct isds_ctx *context, struct isds_message *message);
int isds_list_messages(struct isds_ctx *context, struct isds_message **message);
int isds_find_recipient(struct isds_ctx *context, const struct address *pattern,
        struct isds_address **address);

int isds_message_free(struct isds_message **message);
int isds_address_free(struct isds_address **address);
*/

#endif
