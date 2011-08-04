#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE   /* For getaddrinfo(3) */
#endif

#ifndef _BSD_SOURCE
#define _BSD_SOURCE   /* For NI_MAXHOST */
#endif

#include "../test.h"
#include "isds.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <wait.h>

static const char *username = "douglas";
static const char *password = "42";

static const char *server_error = NULL;

/* Save pointer to static error message if not yet set */
static void set_server_error(const char *message) {
    if (server_error == NULL) {
        server_error = message;
    }
}


/* Creates listening TCP socket on localhost.
 * Returns the socket descriptor or -1. */
static int listen_on_socket(void) {
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
static char *socket2address(int socket) {
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


/* Do the server protocol.
 * Never returns. Terminates by exit(). */
static void server(int server_socket) {
    int client_socket;

    client_socket = accept(server_socket, NULL, NULL);
    fprintf(stderr, "Connection accepted\n");

    close(server_socket);
    exit(EXIT_SUCCESS);
}


/* Start sever in separate process.
 * @server_process is PID of forked server
 * @server_address is automatically allocated TCP address of listening server
 * socket.
 * @return -1 in case of error. */
static int start_server(pid_t *server_process, char **server_address) {
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
        server(server_socket);
        /* Does not return */
    }

    return 0;
}


/* Kill the server process.
 * Return -1 in case of error. */
static int stop_server(pid_t server_process) {
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


static int test_login(const isds_error error, struct isds_ctx *context,
        const char *url, const char *username, const char *password,
        const struct isds_pki_credentials *pki_credentials,
        struct isds_otp *otp) {
    isds_error err;

    err = isds_login(context, url, username, password, pki_credentials, otp);
    if (error != err)
        FAIL_TEST("Wrong return code: expected=%s, returned=%s",
                isds_strerror(error), isds_strerror(err));

    isds_logout(context);
    PASS_TEST;
}

int main(int argc, char **argv) {
    int error;
    pid_t server_process;
    char *server_address = NULL;
    struct isds_ctx *context = NULL;
    char *url = NULL;

    INIT_TEST("server");

    error = start_server(&server_process, &server_address);
    if (error == -1) {
        ABORT_UNIT(server_error);
    }
    if (-1 == test_asprintf(&url, "http://%s/", server_address)) {
        free(server_address);
        stop_server(server_process);
        ABORT_UNIT("Could not format ISDS URL");
    }
    free(server_address);

    if (isds_init()) {
        isds_cleanup();
        free(url);
        stop_server(server_process);
        ABORT_UNIT("isds_init() failed\n");
    }
    context = isds_ctx_create();
    if (!context) {
        isds_cleanup();
        free(url);
        stop_server(server_process);
        ABORT_UNIT("isds_ctx_create() failed\n");
    }

    TEST("valid login", test_login, IE_SUCCESS, context,
            url, username, password, NULL, NULL);

    isds_ctx_free(&context);
    isds_cleanup();
    free(url);
    if (-1 == stop_server(server_process)) {
        ABORT_UNIT(server_error);
    }
    
    SUM_TEST();
}
