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
#include <netdb.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <wait.h>

const char *server_error = NULL;

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

/* Save pointer to static error message if not yet set */
void set_server_error(const char *message) {
    if (server_error == NULL) {
        server_error = message;
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
static void do_ws(int client_socket,
        const struct service_configuration *ws_configuration,
        const struct http_request *request, const char *required_base_path) {
    char *end_point = request->uri; /* Pointer to string in the request */

    if (request->method != HTTP_METHOD_POST) {
        http_send_response_400(client_socket,
                "Regular ISDS web service request must be POST");
        return;
    }

    if (required_base_path != NULL) {
        size_t required_base_path_length = strlen(required_base_path);
        if (strncmp(end_point, required_base_path, required_base_path_length)) {
            http_send_response_400(client_socket,
                    "Request sent to invalid path");
            return;
        }
        end_point += required_base_path_length;
    }

    soap(client_socket, ws_configuration, request->body, request->body_length,
            end_point);
}


/* Do the server protocol.
 * @server_socket is listening TCP socket of the server
 * @server_arguments is pointer to structure:
 * Never returns. Terminates by exit(). */
void server_basic_authentication(int server_socket,
        const void *server_arguments) {
    int client_socket;
    const struct arguments_basic_authentication *arguments =
        (const struct arguments_basic_authentication *) server_arguments;
    struct http_request *request = NULL;
    http_error error;

    if (arguments == NULL) {
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    while (0 <= (client_socket = accept(server_socket, NULL, NULL))) {
        fprintf(stderr, "Connection accepted\n");
        error = http_read_request(client_socket, &request);
        if (error) {
            fprintf(stderr, "Error while reading request\n");
            if (error == HTTP_ERROR_CLIENT)
                http_send_response_400(client_socket, "Error in request");
            else
                http_send_response_500(client_socket,
                        "Error while reading request");
            close(client_socket);
            continue;
        }

        if (request->method == HTTP_METHOD_POST) {
            /* Only POST requests are used in Basic authentication mode */
            if (arguments->username != NULL) {
                if (http_client_authenticates(request)) {
                    switch(http_authenticate_basic(request,
                                arguments->username, arguments->password)) {
                        case HTTP_ERROR_SUCCESS:
                            do_ws(client_socket, arguments->services, request,
                                    ws_base_path_basic);
                            break;
                        case HTTP_ERROR_CLIENT:
                            if (arguments->isds_deviations)
                                http_send_response_401_basic(client_socket);
                            else
                                http_send_response_403(client_socket);
                            break;
                        default:
                            http_send_response_500(client_socket,
                                    "Server error while verifying Basic "
                                    "authentication");
                    }
                } else {
                    http_send_response_401_basic(client_socket);
                }
            } else {
                do_ws(client_socket, arguments->services, request,
                        ws_base_path_basic);
            }
        } else {
            /* HTTP method unsupported per ISDS specification */
            http_send_response_400(client_socket,
                    "Only POST method is allowed");
        }
        http_request_free(&request);


        close(client_socket);
    }

    close(server_socket);
    exit(EXIT_SUCCESS);
}


/* Process first phase of TOTP request */
static void do_as_sendsms(int client_socket, const struct http_request *request,
        const struct arguments_otp_authentication *arguments) {
    if (arguments == NULL) {
        http_send_response_500(client_socket,
                "Third argument of do_as_sendsms() is NULL");
        return;
    }

    if (request->method != HTTP_METHOD_POST) {
        http_send_response_400(client_socket,
                "First phase TOTP request must be POST");
        return;
    }

    if (!http_client_authenticates(request)) {
        http_send_response_401_otp(client_socket,
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
                    http_send_response_400(client_socket,
                            "Missing uri parameter in Request URI");
                    return;
                }
                uri += 5;
                /* Build new location for second OTP phase */ 
                char *location = NULL;
                if (-1 == test_asprintf(&location, "%s%s", as_path_dontsendsms, uri)) {
                    http_send_response_500(client_socket,
                            "Could not build new localtion for "
                            "second OTP phase");
                    return;
                }
                char *terminator = strchr(uri, '&');
                if (NULL != terminator) 
                    location[strlen(as_path_dontsendsms) + (uri - terminator)] = '\0';
                http_send_response_302_totp(client_socket,
                        "authentication.info.totpSended",
                        "=?UTF-8?B?SmVkbm9yw6F6b3bDvSBrw7NkIG9kZXNsw6FuLg==?=",
                        location);
                free(location);
            }
            break;
        case HTTP_ERROR_CLIENT:
            if (arguments->isds_deviations)
                http_send_response_401_otp(client_socket,
                        "totpsendsms",
                        "authentication.error.userIsNotAuthenticated",
                        " Retry: Bad user name or password in first OTP phase.\r\n"
                        " This is very long header\r\n"
                        " which should span to more lines.\r\n"
                        "   Surrounding LWS are meaning-less. ");
            else
                http_send_response_403(client_socket);
            break;
        default:
            http_send_response_500(client_socket,
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
static void do_as_phase_two(int client_socket, const struct http_request *request,
        const struct arguments_otp_authentication *arguments) {
    if (arguments == NULL) {
        http_send_response_500(client_socket,
                "Third argument of do_as_phase_two() is NULL");
        return;
    }

    if (request->method != HTTP_METHOD_POST) {
        http_send_response_400(client_socket,
                "Second phase OTP request must be POST");
        return;
    }

    if (!http_client_authenticates(request)) {
        http_send_response_401_otp(client_socket,
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
                    http_send_response_400(client_socket,
                            "Missing uri parameter in Request URI");
                    return;
                }
                uri += 5;
                /* Build new location for final request */ 
                char *location = NULL;
                if (NULL == (location = strdup(uri))) {
                    http_send_response_500(client_socket,
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
                    http_send_response_500(client_socket,
                            "Could not generate cookie value");
                    free(location);
                    return;
                }
                /* XXX: Add Path parameter to cookie, otherwise
                 * different paths will not match.
                 * FIXME: Domain argument does not work with cURL. Report a bug. */
                http_send_response_302_cookie(client_socket,
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
                http_send_response_401_otp(client_socket,
                        auth_otp_method2string(arguments->method),
                        "authentication.error.userIsNotAuthenticated",
                        " Retry: Bad user name or password in second OTP phase.\r\n"
                        " This is very long header\r\n"
                        " which should span to more lines.\r\n"
                        "   Surrounding LWS are meaning-less. ");
            else
                http_send_response_403(client_socket);
            break;
        default:
            http_send_response_500(client_socket,
                    "Could not verify OTP authentication");
    }
}


/* Process ASWS for changing OTP password request */
/* TODO: Unify with do_as_phase_two(). */
static void do_asws(int client_socket, const struct http_request *request,
        const struct arguments_otp_authentication *arguments) {
    if (arguments == NULL) {
        http_send_response_500(client_socket,
                "Third argument of do_asws() is NULL");
        return;
    }

    if (request->method != HTTP_METHOD_POST) {
        http_send_response_400(client_socket,
                "ASWS request must be POST");
        return;
    }

    if (!http_client_authenticates(request)) {
        http_send_response_401_otp(client_socket,
                auth_otp_method2string(arguments->method),
                "authentication.error.userIsNotAuthenticated",
                "Client did not send any authentication header");
        return;
    }

    switch(http_authenticate_otp(request,
                arguments->username, arguments->password, arguments->otp)) {
        case HTTP_ERROR_SUCCESS:
            do_ws(client_socket, arguments->services, request, NULL);
            break;
        case HTTP_ERROR_CLIENT:
            if (arguments->isds_deviations)
                http_send_response_401_otp(client_socket,
                        auth_otp_method2string(arguments->method),
                        "authentication.error.userIsNotAuthenticated",
                        " Retry: Bad user name or password in second OTP phase.\r\n"
                        " This is very long header\r\n"
                        " which should span to more lines.\r\n"
                        "   Surrounding LWS are meaning-less. ");
            else
                http_send_response_403(client_socket);
            break;
        default:
            http_send_response_500(client_socket,
                    "Could not verify OTP authentication");
    }
}


/* Process OTP session cookie invalidation request */
static void do_as_logout(int client_socket, const struct http_request *request,
        const struct arguments_otp_authentication *arguments) {
    if (arguments == NULL) {
        http_send_response_500(client_socket,
                "Third argument of do_as_logout() is NULL");
        return;
    }

    if (request->method != HTTP_METHOD_GET) {
        http_send_response_400(client_socket,
                "OTP cookie invalidation request must be GET");
        return;
    }

    const char *received_cookie =
        http_find_cookie(request, authorization_cookie_name);

    if (authorization_cookie_value == NULL || received_cookie == NULL ||
            strcmp(authorization_cookie_value, received_cookie)) {
        http_send_response_403(client_socket);
        return;
    }

    /* XXX: Add Path parameter to cookie, otherwise
     * different paths will not match.
     * FIXME: Domain argument does not work with cURL. Report a bug. */
    http_send_response_200_cookie(client_socket,
            authorization_cookie_name,
            "",
            /*http_find_host(request)*/ NULL,
            /*NULL*/"/",
            NULL, 0, NULL);
}


/* Process ISDS WS ping authorized by cookie */
static void do_ws_with_cookie(int client_socket,
        const struct http_request *request,
        const struct arguments_otp_authentication *arguments,
        const char *valid_base_path) {
    const char *received_cookie =
        http_find_cookie(request, authorization_cookie_name);

    if (authorization_cookie_value != NULL && received_cookie != NULL &&
            !strcmp(authorization_cookie_value, received_cookie))
        do_ws(client_socket, arguments->services, request, valid_base_path);
    else
        http_send_response_403(client_socket);
}


/* Do the server protocol with OTP authentication.
 * @server_socket is listening TCP socket of the server
 * @server_arguments is pointer to structure arguments_otp_authentication. It
 * selects OTP method to enable.
 * Never returns. Terminates by exit(). */
void server_otp_authentication(int server_socket,
        const void *server_arguments) {
    int client_socket;
    const struct arguments_otp_authentication *arguments =
        (const struct arguments_otp_authentication *) server_arguments;
    struct http_request *request = NULL;
    http_error error;

    if (arguments == NULL) {
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    while (0 <= (client_socket = accept(server_socket, NULL, NULL))) {
        fprintf(stderr, "Connection accepted\n");
        error = http_read_request(client_socket, &request);
        if (error) {
            fprintf(stderr, "Error while reading request\n");
            if (error == HTTP_ERROR_CLIENT)
                http_send_response_400(client_socket, "Error in request");
            else
                http_send_response_500(client_socket, "Could not read request");
            close(client_socket);
            continue;
        }

        if (arguments->username != NULL) {
            if (arguments->method == AUTH_OTP_HMAC &&
                    !strncmp(request->uri, as_path_hotp, strlen(as_path_hotp))) {
                do_as_phase_two(client_socket, request, arguments);
            } else if (arguments->method == AUTH_OTP_TIME &&
                    !strncmp(request->uri, as_path_sendsms,
                        strlen(as_path_sendsms))) {
                do_as_sendsms(client_socket, request, arguments);
            } else if (arguments->method == AUTH_OTP_TIME &&
                    !strncmp(request->uri, as_path_dontsendsms,
                        strlen(as_path_dontsendsms))) {
                do_as_phase_two(client_socket, request, arguments);
            } else if (!strncmp(request->uri, as_path_logout,
                        strlen(as_path_logout))) {
                do_as_logout(client_socket, request, arguments);
            } else if (arguments->method == AUTH_OTP_TIME &&
                    !strcmp(request->uri, asws_path)) {
                do_asws(client_socket, request, arguments);
            } else if (!strcmp(request->uri, ws_path)) {
                do_ws_with_cookie(client_socket, request, arguments,
                        ws_base_path_otp);
            } else {
                http_send_response_400(client_socket,
                        "Unknown path for TOTP authenticating service");
            }            
        } else {
            if (!strcmp(request->uri, ws_path)) {
                do_ws(client_socket, arguments->services, request,
                        ws_base_path_otp);
            } else {
                http_send_response_400(client_socket,
                        "Unknown path for TOTP authenticating service");
            }            
        }
        http_request_free(&request);


        close(client_socket);
    }

    close(server_socket);
    free(authorization_cookie_value);
    exit(EXIT_SUCCESS);
}


/* Implementation of server that is out of order.
 * It always sends back SOAP Fault with HTTP error 503.
 * @server_socket is listening TCP socket of the server
 * @server_arguments is ununsed pointer
 * Never returns. Terminates by exit(). */
void server_out_of_order(int server_socket,
        const void *server_arguments) {
    int client_socket;
    struct http_request *request = NULL;
    http_error error;
    const char *soap_mime_type = "text/xml"; /* SOAP/1.1 requires text/xml */
    const char *fault = "<?xml version='1.0' encoding='UTF-8'?><SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" xmlns:xsi=\"http://www.w3.org/1999/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/1999/XMLSchema\"><SOAP-ENV:Body><SOAP-ENV:Fault><faultcode xsi:type=\"xsd:string\">Probíhá plánovaná údržba</faultcode><faultstring xsi:type=\"xsd:string\">Omlouváme se všem uživatelům datových schránek za dočasné omezení přístupu do systému datových schránek z důvodu plánované údržby systému. Děkujeme za pochopení.</faultstring></SOAP-ENV:Fault></SOAP-ENV:Body></SOAP-ENV:Envelope>";

    while (0 <= (client_socket = accept(server_socket, NULL, NULL))) {
        fprintf(stderr, "Connection accepted\n");
        error = http_read_request(client_socket, &request);
        if (error) {
            fprintf(stderr, "Error while reading request\n");
            if (error == HTTP_ERROR_CLIENT)
                http_send_response_400(client_socket, "Error in request");
            close(client_socket);
            continue;
        }

        http_send_response_503(client_socket, fault, strlen(fault),
                soap_mime_type);
        http_request_free(&request);

        close(client_socket);
    }

    close(server_socket);
    exit(EXIT_SUCCESS);
}


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
        const void *server_arguments) {
    int server_socket;
    
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

    *server_process = fork();
    if (*server_process == -1) {
        close(server_socket);
        set_server_error("Server could not been forked");
        return -1;
    }

    if (*server_process == 0) {
        server_implementation(server_socket, server_arguments);
        /* Does not return */
    }

    return 0;
}


/* Kill the server process.
 * Return -1 in case of error. */
int stop_server(pid_t server_process) {
    if (server_process <= 0) {
        set_server_error("Invalid server PID to kill");
        return -1;
    }
    if (-1 == kill(server_process, SIGTERM)) {
        set_server_error("Could not terminate server");
        return -1;
    }
    if (-1 == waitpid(server_process, NULL, 0)) {
        set_server_error("Could not wait for server termination");
        return -1;
    }
    return 0;
}

