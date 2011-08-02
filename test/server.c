#include "test.h"

#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE   /* For getaddrinfo(3) */
#endif

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <wait.h>

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
    if (!retval) return -1;

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
    return -1;
}

/* Do the server protocol.
 * Never returns. Terminates by exit(). */
void server(int server_socket) {

    close(server_socket);
    exit(EXIT_SUCCESS);
}


/* Start sever in separate process.
 * Return PID of server or
 * -1 in case of error. */
pid_t start_server(void) {
    int server_socket;
    pid_t server_process;
    
    server_socket = listen_on_socket();
    if (server_socket == -1) return -1;

    server_process = fork();
    if (server_process == -1) {
        close(server_socket);
        return -1;
    }

    if (server_process == 0) {
        server(server_socket);
        /* Does not return */
    }

    return server_process;
}


/* Kill the server process.
 * Return -1 in case of error. */
int stop_server(pid_t server_process) {
    if (-1 == kill(server_process, SIGTERM))
        return -1;
    if (-1 == waitpid(server_process, NULL, 0))
        return -1;
    return 0;
}


int main(int argc, char **argv) {
    int error;
    pid_t server_process;

    INIT_TEST("server");

    server_process = start_server();
    if (server_process == -1) {
        FAILURE_REASON("Could not start server process");
    }

    error = stop_server(server_process);
    if (error == -1) {
        FAILURE_REASON("Could not stop server process");
    }
    
    SUM_TEST();
}
