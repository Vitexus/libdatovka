#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE   /* For getaddrinfo(3) */
#endif

#ifndef _BSD_SOURCE
#define _BSD_SOURCE   /* For NI_MAXHOST */
#endif

#include "../test-tools.h"
#include "http.h"
#include "server.h"
#include "service.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>  /* For EINTR */
#include <netdb.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <wait.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

char *server_error = NULL;

static const char *as_path_hotp = "/as/processLogin?type=hotp&uri=";
static const char *as_path_sendsms = "/as/processLogin?type=totp&sendSms=true&uri=";
static const char *as_path_dontsendsms = "/as/processLogin?type=totp&uri=";
static const char *as_path_logout = "/as/processLogout?uri=";
static const char *asws_path = "/asws/changePassword";
static const char *ws_path = "/apps/DS/dz";

static const char *ws_base_path_basic = "/";
static const char *ws_base_path_otp = "/apps/";

static const char *authorization_cookie_name = "IPCZ-X-COOKIE";
static char *authorization_cookie_value = NULL;

/* TLS stuff */
static gnutls_certificate_credentials_t x509_credentials;
static gnutls_priority_t priority_cache;
static gnutls_dh_params_t dh_parameters;

/* Save error message if not yet set. The message will be duplicated.
 * @message is printf(3) formatting string. */
void set_server_error(const char *message, ...) {
    if (server_error == NULL) {
        va_list ap;
        va_start(ap, message);
        test_vasprintf(&server_error, message, ap);
        va_end(ap);
    }
}


/* Creates listening TCP socket on localhost.
 * Returns the socket descriptor or -1. */
int listen_on_socket(void) {
    int retval;
    struct addrinfo hints;
    struct addrinfo *addresses, *address;
    int fd;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    retval = getaddrinfo("localhost", NULL, &hints, &addresses);
    if (retval) {
        set_server_error("Could not resolve `localhost'");
        return -1;
    }

    for (address = addresses; address != NULL; address = address->ai_next) {
        fd = socket(address->ai_family, address->ai_socktype,
                address->ai_protocol);
        if (fd == -1) continue;

        retval = bind(fd, address->ai_addr, address->ai_addrlen);
        if (retval != 0) continue;

        retval = listen(fd, 0);
        if (retval == 0) {
            freeaddrinfo(addresses);
            return fd;
        }
    }

    freeaddrinfo(addresses);
    set_server_error("Could not start listening on TCP/localhost");
    return -1;
}


/* Format socket address as printable string.
 * @return allocated string or NULL in case of error. */
char *socket2address(int socket) {
    struct sockaddr_storage storage;
    socklen_t length = (socklen_t) sizeof(storage);
    char host[NI_MAXHOST];
    char service[NI_MAXSERV];
    char *address = NULL;

    if (-1 == getsockname(socket, (struct sockaddr *)&storage, &length)) {
        set_server_error("Could not get address of server socket");
        return NULL;
    }

    if (0 != getnameinfo((struct sockaddr *)&storage, length,
                host, sizeof(host), service, sizeof(service),
                NI_NUMERICHOST|NI_NUMERICSERV)) {
        set_server_error("Could not resolve address of server socket");
        return NULL;
    }
    
    if (-1 == test_asprintf(&address,
                (strchr(host, ':') == NULL) ? "%s:%s" : "[%s]:%s",
                host, service)) {
        set_server_error("Could not format server address");
        return NULL;
    }

    return address;
}


/* Process ISDS web service */
static void do_ws(const struct http_connection *connection,
        const struct service_configuration *ws_configuration,
        const struct http_request *request, const char *required_base_path) {
    char *end_point = request->uri; /* Pointer to string in the request */

    if (request->method != HTTP_METHOD_POST) {
        http_send_response_400(connection,
                "Regular ISDS web service request must be POST");
        return;
    }

    if (required_base_path != NULL) {
        size_t required_base_path_length = strlen(required_base_path);
        if (strncmp(end_point, required_base_path, required_base_path_length)) {
            http_send_response_400(connection, "Request sent to invalid path");
            return;
        }
        end_point += required_base_path_length;
    }

    soap(connection, ws_configuration, request->body, request->body_length,
            end_point);
}


/* Do the server protocol.
 * @connection is HTTP connection
 * @server_arguments is pointer to structure:
 * @request is parsed HTTP client request
 * @return 0 to accept new client, return -1 in case of fatal error. */
int server_basic_authentication(const struct http_connection *connection,
        const void *server_arguments, const struct http_request *request) {
    const struct arguments_basic_authentication *arguments =
        (const struct arguments_basic_authentication *) server_arguments;

    if (NULL == arguments || NULL == request) {
        return -1;
    }

    if (request->method == HTTP_METHOD_POST) {
        /* Only POST requests are used in Basic authentication mode */
        if (arguments->username != NULL) {
            if (http_client_authenticates(request)) {
                switch(http_authenticate_basic(request,
                            arguments->username, arguments->password)) {
                    case HTTP_ERROR_SUCCESS:
                        do_ws(connection, arguments->services, request,
                                ws_base_path_basic);
                        break;
                    case HTTP_ERROR_CLIENT:
                        if (arguments->isds_deviations)
                            http_send_response_401_basic(connection);
                        else
                            http_send_response_403(connection);
                        break;
                    default:
                        http_send_response_500(connection,
                                "Server error while verifying Basic "
                                "authentication");
                }
            } else {
                http_send_response_401_basic(connection);
            }
        } else {
            do_ws(connection, arguments->services, request,
                    ws_base_path_basic);
        }
    } else {
        /* HTTP method unsupported per ISDS specification */
        http_send_response_400(connection,
                "Only POST method is allowed");
    }

    return 0;
}


/* Process first phase of TOTP request */
static void do_as_sendsms(const struct http_connection *connection,
        const struct http_request *request,
        const struct arguments_otp_authentication *arguments) {
    if (arguments == NULL) {
        http_send_response_500(connection,
                "Third argument of do_as_sendsms() is NULL");
        return;
    }

    if (request->method != HTTP_METHOD_POST) {
        http_send_response_400(connection,
                "First phase TOTP request must be POST");
        return;
    }

    if (!http_client_authenticates(request)) {
        http_send_response_401_otp(connection,
                "totpsendsms",
                "authentication.error.userIsNotAuthenticated",
                "Client did not send any authentication header");
        return;
    }

    switch(http_authenticate_basic(request,
                arguments->username, arguments->password)) {
        case HTTP_ERROR_SUCCESS: {
                /* Find final URI */
                char *uri = strstr(request->uri, "&uri="); 
                if (uri == NULL) {
                    http_send_response_400(connection,
                            "Missing uri parameter in Request URI");
                    return;
                }
                uri += 5;
                /* Build new location for second OTP phase */ 
                char *location = NULL;
                if (-1 == test_asprintf(&location, "%s%s", as_path_dontsendsms, uri)) {
                    http_send_response_500(connection,
                            "Could not build new localtion for "
                            "second OTP phase");
                    return;
                }
                char *terminator = strchr(uri, '&');
                if (NULL != terminator) 
                    location[strlen(as_path_dontsendsms) + (uri - terminator)] = '\0';
                http_send_response_302_totp(connection,
                        "authentication.info.totpSended",
                        "=?UTF-8?B?SmVkbm9yw6F6b3bDvSBrw7NkIG9kZXNsw6FuLg==?=",
                        location);
                free(location);
            }
            break;
        case HTTP_ERROR_CLIENT:
            if (arguments->isds_deviations)
                http_send_response_401_otp(connection,
                        "totpsendsms",
                        "authentication.error.userIsNotAuthenticated",
                        " Retry: Bad user name or password in first OTP phase.\r\n"
                        " This is very long header\r\n"
                        " which should span to more lines.\r\n"
                        "   Surrounding LWS are meaning-less. ");
            else
                http_send_response_403(connection);
            break;
        default:
            http_send_response_500(connection,
                    "Could not verify Basic authentication");
    }
}


/* Return static string representation of HTTP OTP authentication method.
 * Or NULL in case of error. */
static const char *auth_otp_method2string(enum auth_otp_method method) {
    switch (method) {
        case AUTH_OTP_HMAC: return "hotp";
        case AUTH_OTP_TIME: return "totp";
        default: return NULL;
    }
}


/* Process second phase of OTP request */
static void do_as_phase_two(const struct http_connection *connection,
        const struct http_request *request,
        const struct arguments_otp_authentication *arguments) {
    if (arguments == NULL) {
        http_send_response_500(connection,
                "Third argument of do_as_phase_two() is NULL");
        return;
    }

    if (request->method != HTTP_METHOD_POST) {
        http_send_response_400(connection,
                "Second phase OTP request must be POST");
        return;
    }

    if (!http_client_authenticates(request)) {
        http_send_response_401_otp(connection,
                auth_otp_method2string(arguments->method),
                "authentication.error.userIsNotAuthenticated",
                "Client did not send any authentication header");
        return;
    }

    switch(http_authenticate_otp(request,
                arguments->username, arguments->password, arguments->otp)) {
        case HTTP_ERROR_SUCCESS: {
                /* Find final URI */
                char *uri = strstr(request->uri, "&uri="); 
                if (uri == NULL) {
                    http_send_response_400(connection,
                            "Missing uri parameter in Request URI");
                    return;
                }
                uri += 5;
                /* Build new location for final request */ 
                char *location = NULL;
                if (NULL == (location = strdup(uri))) {
                    http_send_response_500(connection,
                            "Could not build new location for final request");
                    return;
                }
                char *terminator = strchr(location, '&');
                if (NULL != terminator) 
                    *terminator = '\0';
                /* Generate pseudo-random cookie value. This is to prevent
                 * client from reusing the cookie accidentally. We use the
                 * same seed to get reproducible tests. */
                if (-1 == test_asprintf(&authorization_cookie_value, "%d",
                            rand())) {
                    http_send_response_500(connection,
                            "Could not generate cookie value");
                    free(location);
                    return;
                }
                /* XXX: Add Path parameter to cookie, otherwise
                 * different paths will not match.
                 * FIXME: Domain argument does not work with cURL. Report a bug. */
                http_send_response_302_cookie(connection,
                        authorization_cookie_name,
                        authorization_cookie_value,
                        /*http_find_host(request)*/NULL,
                        /*NULL*/"/",
                        location);
                free(location);
            }
            break;
        case HTTP_ERROR_CLIENT:
            if (arguments->isds_deviations)
                http_send_response_401_otp(connection,
                        auth_otp_method2string(arguments->method),
                        "authentication.error.userIsNotAuthenticated",
                        " Retry: Bad user name or password in second OTP phase.\r\n"
                        " This is very long header\r\n"
                        " which should span to more lines.\r\n"
                        "   Surrounding LWS are meaning-less. ");
            else
                http_send_response_403(connection);
            break;
        default:
            http_send_response_500(connection,
                    "Could not verify OTP authentication");
    }
}


/* Process ASWS for changing OTP password requests */
/* FIXME: The ASWS URI hosts two services: for sending TOTP code for password
 * change and for changing OTP password. The problem is the former one is
 * basic-authenticated, the later one is otp-authenticated. But we cannot
 * decide which authentication to enforce without understadning request body. 
 * I will just try both of them to choose the service.
 * But I hope official server implementation does it in more clever way. */
static void do_asws(const struct http_connection *connection,
        const struct http_request *request,
        const struct arguments_otp_authentication *arguments) {
    http_error error;
    if (arguments == NULL) {
        http_send_response_500(connection,
                "Third argument of do_asws() is NULL");
        return;
    }

    if (request->method != HTTP_METHOD_POST) {
        http_send_response_400(connection, "ASWS request must be POST");
        return;
    }

    if (!http_client_authenticates(request)) {
        http_send_response_401_otp(connection,
                auth_otp_method2string(arguments->method),
                "authentication.error.userIsNotAuthenticated",
                "Client did not send any authentication header");
        return;
    }

    /* Try OTP */
    error = http_authenticate_otp(request,
                arguments->username, arguments->password, arguments->otp);
    if (HTTP_ERROR_SUCCESS == error) {
        /* This will be request for password change because OTP
         * authentication succeeded. */
        do_ws(connection, arguments->services, request, NULL);
        return;
    }
    
    if (AUTH_OTP_TIME == arguments->method) {
        /* Try Basic */
        error = http_authenticate_basic(request,
                    arguments->username, arguments->password);
        if (HTTP_ERROR_SUCCESS == error) {
            /* This will be request for time code for password change because
             * basic authentication succeeded. */
            do_ws(connection, arguments->services, request, NULL);
            return;
        }
    }

    if (HTTP_ERROR_CLIENT == error) {
        if (arguments->isds_deviations)
            http_send_response_401_otp(connection,
                    auth_otp_method2string(arguments->method),
                    "authentication.error.userIsNotAuthenticated",
                    " Retry: Bad user name or password in Authorization.\r\n"
                    " This is very long header\r\n"
                    " which should span to more lines.\r\n"
                    "   Surrounding LWS are meaning-less. ");
        else
            http_send_response_403(connection);
        return;
    }

    http_send_response_500(connection,
            "Could not verify OTP authentication");
}


/* Process OTP session cookie invalidation request */
static void do_as_logout(const struct http_connection *connection,
        const struct http_request *request,
        const struct arguments_otp_authentication *arguments) {
    if (arguments == NULL) {
        http_send_response_500(connection,
                "Third argument of do_as_logout() is NULL");
        return;
    }

    if (request->method != HTTP_METHOD_GET) {
        http_send_response_400(connection,
                "OTP cookie invalidation request must be GET");
        return;
    }

    const char *received_cookie =
        http_find_cookie(request, authorization_cookie_name);

    if (authorization_cookie_value == NULL || received_cookie == NULL ||
            strcmp(authorization_cookie_value, received_cookie)) {
        http_send_response_403(connection);
        return;
    }

    /* XXX: Add Path parameter to cookie, otherwise
     * different paths will not match.
     * FIXME: Domain argument does not work with cURL. Report a bug. */
    http_send_response_200_cookie(connection,
            authorization_cookie_name,
            "",
            /*http_find_host(request)*/ NULL,
            /*NULL*/"/",
            NULL, 0, NULL);
}


/* Process ISDS WS ping authorized by cookie */
static void do_ws_with_cookie(const struct http_connection *connection,
        const struct http_request *request,
        const struct arguments_otp_authentication *arguments,
        const char *valid_base_path) {
    const char *received_cookie =
        http_find_cookie(request, authorization_cookie_name);

    if (authorization_cookie_value != NULL && received_cookie != NULL &&
            !strcmp(authorization_cookie_value, received_cookie))
        do_ws(connection, arguments->services, request, valid_base_path);
    else
        http_send_response_403(connection);
}


/* Do the server protocol with OTP authentication.
 * @connection is HTTP connection
 * @server_arguments is pointer to structure arguments_otp_authentication. It
 * selects OTP method to enable.
 * @request is parsed HTTP client requrest
 * @return 0 to accept new client, return -1 in case of fatal error. */
int server_otp_authentication(const struct http_connection *connection,
        const void *server_arguments, const struct http_request *request) {
    const struct arguments_otp_authentication *arguments =
        (const struct arguments_otp_authentication *) server_arguments;

    if (NULL == arguments || NULL == request) {
        return(-1);
    }

    if (arguments->username != NULL) {
        if (arguments->method == AUTH_OTP_HMAC &&
                !strncmp(request->uri, as_path_hotp, strlen(as_path_hotp))) {
            do_as_phase_two(connection, request, arguments);
        } else if (arguments->method == AUTH_OTP_TIME &&
                !strncmp(request->uri, as_path_sendsms,
                    strlen(as_path_sendsms))) {
            do_as_sendsms(connection, request, arguments);
        } else if (arguments->method == AUTH_OTP_TIME &&
                !strncmp(request->uri, as_path_dontsendsms,
                    strlen(as_path_dontsendsms))) {
            do_as_phase_two(connection, request, arguments);
        } else if (!strncmp(request->uri, as_path_logout,
                    strlen(as_path_logout))) {
            do_as_logout(connection, request, arguments);
        } else if (!strcmp(request->uri, asws_path)) {
            do_asws(connection, request, arguments);
        } else if (!strcmp(request->uri, ws_path)) {
            do_ws_with_cookie(connection, request, arguments,
                    ws_base_path_otp);
        } else {
            http_send_response_400(connection,
                    "Unknown path for OTP authenticating service");
        }            
    } else {
        if (!strcmp(request->uri, ws_path)) {
            do_ws(connection, arguments->services, request,
                    ws_base_path_otp);
        } else {
            http_send_response_400(connection,
                    "Unknown path for OTP authenticating service");
        }            
    }

    return 0;
}


/* Implementation of server that is out of order.
 * It always sends back SOAP Fault with HTTP error 503.
 * @connection is HTTP connection
 * @server_arguments is ununsed pointer
 * @request is parsed HTTP client request
 * @return 0 to accept new client, return -1 in case of fatal error. */
int server_out_of_order(const struct http_connection *connection,
        const void *server_arguments, const struct http_request *request) {
    const char *soap_mime_type = "text/xml"; /* SOAP/1.1 requires text/xml */
    const char *fault = "<?xml version='1.0' encoding='UTF-8'?><SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" xmlns:xsi=\"http://www.w3.org/1999/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/1999/XMLSchema\"><SOAP-ENV:Body><SOAP-ENV:Fault><faultcode xsi:type=\"xsd:string\">Probíhá plánovaná údržba</faultcode><faultstring xsi:type=\"xsd:string\">Omlouváme se všem uživatelům datových schránek za dočasné omezení přístupu do systému datových schránek z důvodu plánované údržby systému. Děkujeme za pochopení.</faultstring></SOAP-ENV:Fault></SOAP-ENV:Body></SOAP-ENV:Envelope>";

    http_send_response_503(connection, fault, strlen(fault),
            soap_mime_type);
    return 0;
}


/* Call-back for HTTP to receive data from socket
 * API is equivalent to recv(2) except automatic interrupt handling. */
static ssize_t recv_plain(const struct http_connection *connection,
        void *buffer, size_t length) {
    ssize_t retval;
    do {
        retval = recv(connection->socket, buffer, length, 0);
    } while (-1 == retval && EINTR == errno);
    return retval;
}


/* Call-back for HTTP to sending data to socket
 * API is equivalent to send(2) except automatic interrupt handling. */
static ssize_t send_plain(const struct http_connection *connection,
        const void *buffer, size_t length) {
    ssize_t retval;
    do {
        retval = send(connection->socket, buffer, length, MSG_NOSIGNAL);
    } while (-1 == retval && EINTR == errno);
    return retval;
}


/* Call-back for HTTP to receive data from TLS socket
 * API is equivalent to recv(2) except automatic interrupt handling. */
static ssize_t recv_tls(const struct http_connection *connection,
        void *buffer, size_t length) {
    ssize_t retval;
    do {
        retval = gnutls_record_recv(connection->callback_data, buffer, length);
        if (GNUTLS_E_REHANDSHAKE == retval) {
            int error;
            do {
                error = gnutls_handshake(connection->callback_data);
            } while (error < 0 && !gnutls_error_is_fatal(error));
            if (error < 0) {
                fprintf(stderr, "TLS rehandshake failed: %s\n",
                        gnutls_strerror(error));
                return -1;
            }
        }
    } while (GNUTLS_E_INTERRUPTED == retval || GNUTLS_E_AGAIN == retval);
    return (retval < 0) ? -1 : retval;
}


/* Call-back for HTTP to sending data to TLS socket
 * API is equivalent to send(2) except automatic interrupt handling. */
static ssize_t send_tls(const struct http_connection *connection,
        const void *buffer, size_t length) {
    ssize_t retval;
    do {
        retval = gnutls_record_send(connection->callback_data, buffer, length);
    } while (GNUTLS_E_INTERRUPTED == retval || GNUTLS_E_AGAIN == retval);
    return (retval < 0) ? -1 : retval;
}


/* Call-back fot GnuTLS to receive data from TCP socket.
 * We override default implementation to unify passing http_connection as
 * @context. Also otherwise there is need for ugly type cast from integer to
 * pointer. */
static ssize_t tls_pull(gnutls_transport_ptr_t context, void* buffer,
        size_t length) {
    return recv( ((struct http_connection*)context)->socket,
            buffer, length, 0);
}


/* Call-back fot GnuTLS to send data to TCP socket.
 * GnuTLS does not call send(2) with MSG_NOSIGNAL, we must do it manually. */
static ssize_t tls_push(gnutls_transport_ptr_t context, const void* buffer,
        size_t length) {
    return send( ((struct http_connection*)context)->socket,
            buffer, length, MSG_NOSIGNAL);
}


/* Verify client certificate from current TLS session.
 * @tls_session is session in TLS handshake when client sent certificate
 * @return 0 for acceptance, return non-0 for denial. */
static int tls_verify_client(gnutls_session_t tls_session) {
    const gnutls_datum_t *chain; /* Pointer to static data */
    unsigned int chain_length;
    gnutls_x509_crt_t certificate;
    gnutls_datum_t certificate_text;
    unsigned int status;
    int error;

    /* Obtain client's certificate chain */
    chain = gnutls_certificate_get_peers(tls_session, &chain_length);
    if (NULL == chain) {
        fprintf(stderr, "Error while obtaining client's certificate\n");
        return -1;
    }
    if (chain_length < 1) {
        fprintf(stderr, "Client did not send any certificate\n");
        return -1;
    }

    /* Print client's certificate */
    error = gnutls_x509_crt_init(&certificate);
    if (error) {
        fprintf(stderr, "Could not initialize certificate storage: %s\n",
                gnutls_strerror(error));
        return -1;
    }
    error = gnutls_x509_crt_import(certificate, chain,
            GNUTLS_X509_FMT_DER);
    if (error) {
        fprintf(stderr, "Could not parse client's X.509 certificate: %s\n",
                gnutls_strerror(error));
        gnutls_x509_crt_deinit(certificate);
        return -1;
    }
    error = gnutls_x509_crt_print(certificate, GNUTLS_CRT_PRINT_ONELINE,
            &certificate_text);
    if (error) {
        fprintf(stderr, "Could not print client's certificate: %s\n",
                gnutls_strerror(error));
        gnutls_x509_crt_deinit(certificate);
        return -1;
    }
    fprintf(stderr, "Client sent certificate: %s\n", certificate_text.data);
    gnutls_free(certificate_text.data);
    gnutls_x509_crt_deinit(certificate);

    /* Verify certificate signature and path */
    error = gnutls_certificate_verify_peers2(tls_session, &status);
    if (error) {
        fprintf(stderr, "Could not verify client's certificate: %s\n",
                gnutls_strerror(error));
        return -1;
    }
    if (status) {
        fprintf(stderr, "Client's certificate is not valid.\n");
        return -1;
    } else {
        fprintf(stderr, "Client's certificate is valid.\n");
    }
    return 0;
}


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
        int (*server_implementation)(const struct http_connection *,
            const void *, const struct http_request *),
        const void *server_arguments, const struct tls_authentication *tls) {
    int server_socket;
    int error;
 
    if (server_address == NULL) {
        set_server_error("start_server(): Got invalid server_address pointer");
        return -1;
    }
    *server_address = NULL;

    if (server_process == NULL) {
        set_server_error("start_server(): Got invalid server_process pointer");
        return -1;
    }

    server_socket = listen_on_socket();
    if (server_socket == -1) {
        set_server_error("Could not create listening socket");
        return -1;
    }

    *server_address = socket2address(server_socket);
    if (*server_address == NULL) {
        close(server_socket);
        set_server_error("Could not format address of listening address");
        return -1;
    }

    if (NULL != tls && NULL == tls->server_certificate) {
        /* XXX: X.509 TLS server requires server certificate. */
        tls = NULL;
    }
    if (NULL != tls) {
        const char *error_position;
        if ((error = gnutls_global_init())) {
            close(server_socket);
            set_server_error("Could not initialize GnuTLS: %s",
                    gnutls_strerror(error));
            return -1;
        }
        if ((error =
                gnutls_certificate_allocate_credentials(&x509_credentials))) {
            close(server_socket);
            gnutls_global_deinit();
            set_server_error("Could not allocate X.509 credentials: %s",
                    gnutls_strerror(error));
            return -1;
        }
        if (NULL != tls->authority_certificate) {
            if (0 > (error = gnutls_certificate_set_x509_trust_file(
                            x509_credentials, tls->authority_certificate,
                            GNUTLS_X509_FMT_PEM))) {
                close(server_socket);
                gnutls_certificate_free_credentials(x509_credentials);
                gnutls_global_deinit();
                set_server_error("Could not load authority certificate `%s': %s",
                        tls->authority_certificate, gnutls_strerror(error));
                return -1;
            }
        }
        if ((error = gnutls_certificate_set_x509_key_file(x509_credentials,
                    tls->server_certificate, tls->server_key,
                    GNUTLS_X509_FMT_PEM))) {
            close(server_socket);
            gnutls_certificate_free_credentials(x509_credentials);
            gnutls_global_deinit();
            set_server_error("Could not load server certificate or "
                    "private key `%s': %s", tls->server_certificate,
                    gnutls_strerror(error));
            return -1;
        }
        if ((error = gnutls_priority_init(&priority_cache,
                    "PERFORMANCE", &error_position))) {
            close(server_socket);
            gnutls_certificate_free_credentials(x509_credentials);
            gnutls_global_deinit();
            if (error == GNUTLS_E_INVALID_REQUEST) {
                set_server_error("Could not set TLS algorithm preferences: "
                            "%s Error at `%s'.",
                        gnutls_strerror(error), error_position);
                set_server_error("Could not set TLS algorithm preferences: %s",
                        gnutls_strerror(error));
            }
            return -1;
        }
        /* XXX: priority_cache is linked from x509_credentials now.
         * Deinitialization must free x509_credentials before priority_cache. */

        if ((error = gnutls_dh_params_init(&dh_parameters))) {
            close(server_socket);
            gnutls_certificate_free_credentials(x509_credentials);
            gnutls_priority_deinit(priority_cache);
            gnutls_global_deinit();
            set_server_error("Could not allocate Diffie-Hellman parameters: "
                    "%s", gnutls_strerror(error));
            return -1;
        }
        if ((error = gnutls_dh_params_generate2(dh_parameters,
                gnutls_sec_param_to_pk_bits(GNUTLS_PK_DH,
                    GNUTLS_SEC_PARAM_LOW)))) {
            close(server_socket);
            gnutls_certificate_free_credentials(x509_credentials);
            gnutls_priority_deinit(priority_cache);
            gnutls_dh_params_deinit(dh_parameters);
            gnutls_global_deinit();
            set_server_error("Could not generate Diffie-Hellman parameters: %s",
                    gnutls_strerror(error));
            return -1;
        }
        gnutls_certificate_set_dh_params(x509_credentials, dh_parameters);
        /* XXX: dh_parameters are linked from x509_credentials now.
         * Deinitialization must free x509_credentials before dh_parameters. */
    }

    *server_process = fork();
    if (*server_process == -1) {
        close(server_socket);
        if (NULL != tls) {
            gnutls_certificate_free_credentials(x509_credentials);
            gnutls_priority_deinit(priority_cache);
            gnutls_dh_params_deinit(dh_parameters);
            gnutls_global_deinit();
        }
        set_server_error("Server could not been forked");
        return -1;
    }

    if (*server_process == 0) {
        int client_socket;
        gnutls_session_t tls_session;
        struct http_connection connection;
        struct http_request *request = NULL;
        http_error error;
        int terminate = 0;

        while (!terminate) {
            if (NULL != tls) {
                if ((error = gnutls_init(&tls_session, GNUTLS_SERVER))) {
                    set_server_error("Could not initialize TLS session: %s",
                            gnutls_strerror(error));
                    terminate = -1;
                    continue;
                }
                if ((error = gnutls_priority_set(tls_session,
                                priority_cache))) {
                    set_server_error(
                            "Could not set priorities to TLS session: %s",
                            gnutls_strerror(error));
                    terminate = -1;
                    gnutls_deinit(tls_session);
                    continue;
                }
                if ((error = gnutls_credentials_set(tls_session,
                                GNUTLS_CRD_CERTIFICATE, x509_credentials))) {
                    set_server_error("Could not set X509 credentials to TLS "
                            "session: %s", gnutls_strerror(error));
                    terminate = -1;
                    gnutls_deinit(tls_session);
                    continue;
                }
                /* XXX: Credentials are linked from session now.
                 * Deinitializition must free session before x509_credentials.
                 */
                if (NULL != tls->client_name) {
                    /* Require client certificate */
                    gnutls_certificate_server_set_request(tls_session,
                            GNUTLS_CERT_REQUIRE);
                    /* And verify it in TLS handshake */
                    gnutls_certificate_set_verify_function(x509_credentials,
                         tls_verify_client);
                } 
            }

            if (0 > (client_socket = accept(server_socket, NULL, NULL))) {
                terminate = -1;
                if (NULL != tls)
                    gnutls_deinit(tls_session);
                continue;
            }
            fprintf(stderr, "Connection accepted\n");
            connection.socket = client_socket;

            if (NULL == tls) {
                connection.recv_callback = recv_plain;
                connection.send_callback = send_plain;
                connection.callback_data = NULL;
            } else {
                connection.recv_callback = recv_tls;
                connection.send_callback = send_tls;
                connection.callback_data = tls_session;
                gnutls_transport_set_pull_function(tls_session, tls_pull);
                gnutls_transport_set_push_function(tls_session, tls_push);
                gnutls_transport_set_ptr2(tls_session,
                        &connection, &connection);
                do {
                    error = gnutls_handshake(tls_session);
                } while (error < 0 && !gnutls_error_is_fatal(error));
                if (error < 0) {
                    fprintf(stderr, "TLS handshake failed: %s\n",
                            gnutls_strerror(error));
                    close(client_socket);
                    gnutls_deinit(tls_session);
                    continue;
                }
            }

            error = http_read_request(&connection, &request);
            if (error) {
                fprintf(stderr, "Error while reading request\n");
                if (error == HTTP_ERROR_CLIENT)
                    http_send_response_400(&connection, "Error in request");
                else
                    http_send_response_500(&connection,
                            "Could not read request");
                close(client_socket);
                if (NULL != tls)
                    gnutls_deinit(tls_session);
                continue;
            }

            terminate = server_implementation(&connection, server_arguments,
                    request);

            http_request_free(&request);
            close(client_socket);
            if (NULL != tls) {
                gnutls_deinit(tls_session);
            }
        }

        close(server_socket);
        free(authorization_cookie_value);
        exit(EXIT_SUCCESS);
        /* Does not return */
    }

    return 0;
}


/* Kill the server process.
 * Return 0. Return -1 if server could not been stopped. Return 1 if server
 * crashed. */
int stop_server(pid_t server_process) {
    int status;
    if (server_process <= 0) {
        set_server_error("Invalid server PID to kill");
        return -1;
    }
    if (-1 == kill(server_process, SIGTERM)) {
        set_server_error("Could not terminate server");
        return -1;
    }
    if (-1 == waitpid(server_process, &status, 0)) {
        set_server_error("Could not wait for server termination");
        return -1;
    }
    if (WIFSIGNALED(status) && WTERMSIG(status) != SIGTERM) {
        set_server_error("Server terminated by signal %d violently",
                WTERMSIG(status));
        return 1;
    }
    return 0;
}

