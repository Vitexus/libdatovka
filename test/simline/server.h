#ifndef __ISDS_SERVER_H
#define __ISDS_SERVER_H

extern const char *server_error;

/* Save pointer to static error message if not yet set */
void set_server_error(const char *message);


/* Creates listening TCP socket on localhost.
 * Returns the socket descriptor or -1. */
int listen_on_socket(void);


/* Format socket address as printable string.
 * @return allocated string or NULL in case of error. */
char *socket2address(int socket);


struct arguments_basic_authentication {
    const char *username;   /* Sets required user name server has to require.
                               Set NULL to disable HTTP authentication. */
    const char *password;   /* sets required password server has to require */
    _Bool isds_deviations;  /* is flag to set conformance level. If false,
                               server is compliant to standards (HTTP, SOAP)
                               if not conflicts with ISDS specification.
                               Otherwise server mimics real ISDS implementation
                               as much as possible. */
};

/* Do the server protocol.
 * @server_socket is listening TCP socket of the server
 * @server_arguments is pointer to structure:
 * Never returns. Terminates by exit(). */
void server_basic_authentication(int server_socket,
        const void *server_arguments);


/* One-time password authentication method */
enum auth_otp_method {
    AUTH_OTP_HMAC = 0,  /* HMAC-based OTP */
    AUTH_OTP_TIME       /* Time-based OTP */
};

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
};

/* Do the server protocol with OTP authentication.
 * @server_socket is listening TCP socket of the server
 * @server_arguments is pointer to structure arguments_otp_authentication. It
 * selects OTP method to enable.
 * Never returns. Terminates by exit(). */
void server_otp_authentication(int server_socket,
        const void *server_arguments);

/* Implementation of server that is out of order.
 * It always sends back SOAP Fault with HTTP error 503.
 * @server_socket is listening TCP socket of the server
 * @server_arguments is ununsed pointer
 * Never returns. Terminates by exit(). */
void server_out_of_order(int server_socket,
        const void *server_arguments);


/* Start sever in separate process.
 * @server_process is PID of forked server
 * @server_address is automatically allocated TCP address of listening server
 * @username sets required user name server has to require. Set NULL to
 * disable HTTP authentication.
 * @password sets required password server has to require
 * socket.
 * @isds_deviations is flag to set conformance level. If false, server is
 * compliant to standards (HTTP, SOAP) if not conflicts with ISDS
 * specification. Otherwise server mimics real ISDS implementation as much
 * as possible.
 * @return -1 in case of error. */
int start_server(pid_t *server_process, char **server_address,
        void (*server_implementation)(int, const void *),
        const void *server_arguments);


/* Kill the server process.
 * Return -1 in case of error. */
int stop_server(pid_t server_process);

#endif
