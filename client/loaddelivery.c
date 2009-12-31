#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <isds.h>
#include "common.h"


int main(int argc, char **argv) {
    struct isds_ctx *ctx = NULL;
    isds_error err;
    
    setlocale(LC_ALL, "");

    err = isds_init();
    if (err) {
        printf("isds_init() failed: %s\n", isds_strerror(err));
        exit(EXIT_FAILURE);
    }

    isds_set_logging(ILF_ALL & ~ILF_HTTP, ILL_ALL);

    ctx = isds_ctx_create();
    if (!ctx) {
        printf("isds_ctx_create() failed");
    }


    {
        struct isds_message *message = NULL;
        void *buffer;
        int fd;
        struct stat file_info;
        size_t pages;

        fd = open("server/messages/signed_delivered-DD_170272.zfo", O_RDONLY);
        if (fd == -1) {
            perror("Could not open file with signed delivery info");
            isds_ctx_free(&ctx);
            isds_cleanup();
            exit(EXIT_FAILURE);
        }

        if (-1 == fstat(fd, &file_info)) {
            perror("Could not get file size");
            close(fd);
            isds_ctx_free(&ctx);
            isds_cleanup();
            exit(EXIT_FAILURE);
        }

        buffer = mmap(NULL, file_info.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (buffer == MAP_FAILED) {
            perror("Could not map signed delivery info from file to memmory");
            close(fd);
            isds_ctx_free(&ctx);
            isds_cleanup();
            exit(EXIT_FAILURE);
        }

        printf("Loading signed delivery info\n");
        err = isds_load_signed_delivery_info(ctx, buffer, file_info.st_size,
                &message, BUFFER_DONT_STORE);
        if (err)
            printf("isds_load_signed_delivery_info() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        else {
            printf("isds_load_signed_delivery_info() succeeded:\n");
            print_envelope(message->envelope);
        }

        isds_message_free(&message);
        {
            long int page_size = sysconf(_SC_PAGE_SIZE);
            pages = (file_info.st_size % page_size) ?
                ((file_info.st_size / page_size) + 1) * page_size:
                file_info.st_size;
        }
        munmap(buffer, pages);
        close(fd);
    }


    err = isds_ctx_free(&ctx);
    if (err) {
        printf("isds_ctx_free() failed: %s\n", isds_strerror(err));
    }


    err = isds_cleanup();
    if (err) {
        printf("isds_cleanup() failed: %s\n", isds_strerror(err));
    }

    exit (EXIT_SUCCESS);
}
