#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE   /* For getaddrinfo(3) */
#endif

#ifndef _BSD_SOURCE
#define _BSD_SOURCE   /* For NI_MAXHOST */
#endif

#include "../test-tools.h"
#include "http.h"

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
        const void *server_arguments) {
    int client_socket;
    const struct arguments_basic_authentication *arguments =
        (const struct arguments_basic_authentication *) server_arguments;
    struct http_request *request = NULL;
    http_error error;
    const char *soap_mime_type = "text/xml"; /* SOAP/1.1 requires text/xml */
    const char *pong = "<?xml version='1.0' encoding='utf-8'?><SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\"><SOAP-ENV:Body><q:DummyOperationResponse xmlns:q=\"http://isds.czechpoint.cz/v20\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"><q:dmStatus><q:dmStatusCode>0000</q:dmStatusCode><q:dmStatusMessage>Provedeno úspěšně.</q:dmStatusMessage></q:dmStatus></q:DummyOperationResponse></SOAP-ENV:Body></SOAP-ENV:Envelope>";

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
                http_send_response_400(client_socket);
            else
                http_send_response_500(client_socket);
            close(client_socket);
            continue;
        }

        if (arguments->username != NULL) {
            if (http_client_authenticates(request)) {
                switch(http_authenticate_basic(request,
                            arguments->username, arguments->password)) {
                    case HTTP_ERROR_SUCCESS:
                        http_send_response_200(client_socket,
                                pong, strlen(pong), soap_mime_type);
                        break;
                    case HTTP_ERROR_CLIENT:
                        if (arguments->isds_deviations)
                            http_send_response_401_basic(client_socket);
                        else
                            http_send_response_403(client_socket);
                        break;
                    default:
                        http_send_response_500(client_socket);
                }
            } else {
                http_send_response_401_basic(client_socket);
            }
        } else {
            http_send_response_200(client_socket,
                    pong, strlen(pong), soap_mime_type);
        }
        http_request_free(&request);


        close(client_socket);
    }

    close(server_socket);
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
                http_send_response_400(client_socket);
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

