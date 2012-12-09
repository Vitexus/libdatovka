#ifndef __ISDS_SERVER_H
#define __ISDS_SERVER_H

#include <sys/types.h> /* For pid_t */
#include "server_types.h"
#include "services.h"

struct http_request;    /* Declare opaque not to export http.h */
extern char *server_error;

/* Save error message if not yet set. The message will be duplicated.
 * @message is printf(3) formatting string. */
void set_server_error(const char *message, ...);


/* Creates listening TCP socket on localhost.
 * Returns the socket descriptor or -1. */
int listen_on_socket(void);


/* Format socket address as printable string.
 * @return allocated string or NULL in case of error. */
char *socket2address(int socket);


struct tls_authentication {
    const char *authority_certificate;  /* PEM CA certificate file name */
    const char *server_certificate;     /* PEM server certificate file name */
    const char *server_key;             /* PEM server private key file name */
    const char *client_name;            /* Client distinguished name.
                                           NULL if you do not want to
                                           authenticate a client using X.509 */
};


struct arguments_basic_authentication {
    const char *username;   /* Sets required user name server has to require.
                               Set NULL to disable HTTP authentication. */
    const char *password;   /* sets required password server has to require */
    _Bool isds_deviations;  /* is flag to set conformance level. If false,
                               server is compliant to standards (HTTP, SOAP)
                               if not conflicts with ISDS specification.
                               Otherwise server mimics real ISDS implementation
                               as much as possible. */
    const struct service_configuration *services;   /* Array of enabled
                                                       services. Last name must
                                                       be SERVICE_END. */
};

/* Do the server protocol.
 * @client_socket is accpeted TCP socket
 * @server_arguments is pointer to structure:
 * @request is parsed HTTP client request
 * @return 0 to accept new client, return -1 in case of fatal error. */
int server_basic_authentication(int client_socket,
        const void *server_arguments, const struct http_request *request);


struct arguments_otp_authentication {
    enum auth_otp_method method;    /* Selects OTP method to enable */
    const char *username;   /* Sets required user name server has to require.
                               Set NULL to disable HTTP authentication. */
    const char *password;   /* Sets password server has to require */
    const char *otp;        /* Sets OTP code server has to requiere */ 
    _Bool isds_deviations;  /* Is flag to set conformance level. If false,
                               server is compliant to standards (HTTP, SOAP)
                               if not conflicts with ISDS specification.
                               Otherwise server mimics real ISDS implementation
                               as much as possible. */
    const struct service_configuration *services;   /* Array of enabled
                                                       services. Last name must
                                                       be SERVICE_END. */
};

/* Do the server protocol with OTP authentication.
 * @client_socket is accepted TCP socket
 * @server_arguments is pointer to structure arguments_otp_authentication. It
 * selects OTP method to enable.
 * @request is parsed HTTP client requrest
 * @return 0 to accept new client, return -1 in case of fatal error. */
int server_otp_authentication(int client_socket,
        const void *server_arguments, const struct http_request *request);

/* Implementation of server that is out of order.
 * It always sends back SOAP Fault with HTTP error 503.
 * @client_socket is accepted TCP socket
 * @server_arguments is ununsed pointer
 * @request is parsed HTTP client request
 * @return 0 to accept new client, return -1 in case of fatal error. */
int server_out_of_order(int client_socket,
        const void *server_arguments, const struct http_request *request);


/* Start sever in separate process.
 * @server_process is PID of forked server
 * @server_address is automatically allocated TCP address of listening server
 * @server_implementation points to kind of server to implement. Valid values
 * are addresses of server_basic_authentication(),
 * server_otp_authentication(), or server_out_of_order().
 * @server_arguments is pointer to argument pass to @server_implementation. It
 * usually contains:
 *  @username sets required user name server has to require. Set NULL to
 *  disable HTTP authentication.
 *  @password sets required password server has to require
 *  socket.
 *  @isds_deviations is flag to set conformance level. If false, server is
 *  compliant to standards (HTTP, SOAP) if not conflicts with ISDS
 *  specification. Otherwise server mimics real ISDS implementation as much
 *  as possible.
 * @tls sets TLS layer. Pass NULL for plain HTTP.
 * @return -1 in case of error. */
int start_server(pid_t *server_process, char **server_address,
        int (*server_implementation)(int, const void *,
            const struct http_request *),
        const void *server_arguments, const struct tls_authentication *tls);


/* Kill the server process.
 * Return -1 in case of error. */
int stop_server(pid_t server_process);

#endif
