#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <time.h>
#include <string.h>
#include <libdatovka/isds.h>
#include "common.h"


int main(int argc, char **argv) {
    struct isds_ctx *ctx = NULL;
    isds_error err;
    struct isds_document document;
    int fd;
    char *id = NULL;
    struct tm *date = NULL;
    
    setlocale(LC_ALL, "");

    if (argc < 3) {
        fprintf(stderr, "Usage: program FILE DESCRIPTION\n");
        exit(EXIT_FAILURE);
    }

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

    err = isds_set_timeout(ctx, 10000);
    if (err) {
        printf("isds_set_timeout() failed: %s\n", isds_strerror(err));
    }


    /* Load document */ 
    memset(&document, 0, sizeof(document));
    if (mmap_file(argv[1], &fd, &document.data, &document.data_length)) {
        fprintf(stderr, "Could not map file with document");
        isds_ctx_free(&ctx);
        isds_cleanup();
        exit(EXIT_FAILURE);
    }
    document.dmFileDescr = argv[2];


    /* Submit document for conversion  */
    printf("Submitting document for authorized conversion: "
            "content=%s, description=%s\n", argv[1], argv[2]);
    err = czp_convert_document(ctx, &document, &id, &date);
    if (err)
        printf("czp_convert_document() failed: %s: %s\n",
                isds_strerror(err), isds_long_message(ctx));
    else {
        printf("czp_convert_document() succeeded:\n");
        printf("\tidentifier = %s\n", id);
        printf("\tsubmit date = ");
        print_date(date);
    }

    free(id);
    free(date);
    munmap_file(fd, document.data, document.data_length);



    err = czp_close_connection(ctx);
    if (err) {
        printf("czp_close_connetion() failed: %s\n", isds_strerror(err));
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
