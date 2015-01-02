#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <time.h>
#include <string.h>
#include <isds.h>
#include "common.h"

isds_error compute_message_hash(struct isds_ctx *ctx,
        struct isds_message *message) {
    isds_error err;
    struct isds_hash *old_hash;

    /* Detach original hash */
    old_hash = message->envelope->hash;
    message->envelope->hash = NULL;

    /* Recalculate hash */
    printf("Calculating message hash\n");
    err = isds_compute_message_hash(ctx, message, old_hash->algorithm);
    if (err) 
        printf("isds_compute_message_hash() failed: %s: %s\n",
                isds_strerror(err), isds_long_message(ctx));
    else {
        printf("isds_compute_message_hash() succeeded:\n");

        printf("Stored hash   = ");
        print_hash(old_hash);

        printf("Computed hash = ");
        print_hash(message->envelope->hash);

        /* Compare hashes */
        compare_hashes(old_hash, message->envelope->hash);
    }

    isds_hash_free(&old_hash);
    return err;
}


int main(void) {
    struct isds_ctx *ctx = NULL;
    isds_error err;
    
    setlocale(LC_ALL, "");

    err = isds_init();
    if (err) {
        printf("isds_init() failed: %s\n", isds_strerror(err));
        exit(EXIT_FAILURE);
    }

    /*isds_set_logging(ILF_ALL & ~ILF_HTTP, ILL_ALL);*/

    ctx = isds_ctx_create();
    if (!ctx) {
        printf("isds_ctx_create() failed");
    }


    {
        /* Load plain received message */
        struct isds_message *message = NULL;
        void *buffer;
        int fd;
        size_t length;
        
        if (mmap_file(SRCDIR "/server/messages/received_message-151916.xml",
                &fd, &buffer, &length)) {
            fprintf(stderr, "Could not map file with plain received message\n");
            isds_ctx_free(&ctx);
            isds_cleanup();
            exit(EXIT_FAILURE);
        }

        printf("Loading plain received message\n");
        err = isds_load_message(ctx, RAWTYPE_INCOMING_MESSAGE,
                buffer, length, &message, BUFFER_COPY);
        if (err) 
            printf("isds_load_message() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        else {
            printf("isds_load_message() succeeded\n");
            compute_message_hash(ctx, message);
        }

        isds_message_free(&message);
        munmap_file(fd, buffer, length);
    }


    {
        /* Load plain signed message */
        struct isds_message *message = NULL;
        void *buffer;
        int fd;
        size_t length;
        
        if (mmap_file(SRCDIR "/server/messages/sent_message-206720.xml",
                &fd, &buffer, &length)) {
            fprintf(stderr, "Could not map file with plain signed message\n");
            isds_ctx_free(&ctx);
            isds_cleanup();
            exit(EXIT_FAILURE);
        }

        printf("Loading plain signed sent message\n");
        err = isds_load_message(ctx, RAWTYPE_PLAIN_SIGNED_OUTGOING_MESSAGE,
                buffer, length, &message, BUFFER_COPY);
        if (err) 
            printf("isds_load_message() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        else {
            printf("isds_load_message() succeeded\n");
            compute_message_hash(ctx, message);
        }

        isds_message_free(&message);
        munmap_file(fd, buffer, length);
    }


    {
        /* Load CMS signed message */
        struct isds_message *message = NULL;
        void *buffer;
        int fd;
        size_t length;
        
        if (mmap_file(SRCDIR "/server/messages/signed_sent_message-151874.zfo",
                &fd, &buffer, &length)) {
            fprintf(stderr, "Could not map file with CMS signed message\n");
            isds_ctx_free(&ctx);
            isds_cleanup();
            exit(EXIT_FAILURE);
        }

        printf("Loading CMS signed sent message\n");
        err = isds_load_message(ctx, RAWTYPE_CMS_SIGNED_OUTGOING_MESSAGE,
                buffer, length, &message, BUFFER_COPY);
        if (err) 
            printf("isds_load_message() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        else {
            printf("isds_load_message() succeeded\n");
            compute_message_hash(ctx, message);
        }

        isds_message_free(&message);
        munmap_file(fd, buffer, length);
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
