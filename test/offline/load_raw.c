#include "../test.h"
#include "crypto.h"
#include <string.h>


struct test {
    char *name;
    char *file;
    isds_raw_type type;
    _Bool should_pass;
};


static int test_load_message(struct isds_ctx *context,
        const struct test *test) {
    isds_error err;
    int fd;
    void *buffer = NULL;
    size_t length = 0;
    struct isds_message *message = NULL;

    if (!test) return 1;

    if (test_mmap_file(test->file, &fd, &buffer, &length))
        FAIL_TEST("Could not load test data from `%s' file", test->file);

    if (!test->should_pass)
        /* Do not log expected XML parser failures */
        isds_set_logging(ILF_XML, ILL_NONE);
    else
        isds_set_logging(ILF_ALL, ILL_WARNING);

    err = isds_load_message(context, test->type, buffer, length,
            &message, BUFFER_DONT_STORE);
    isds_message_free(&message);
    test_munmap_file(fd, buffer, length);

    if (test->should_pass) {
        if (err)
            FAIL_TEST("Message loading should succeed: got=%s",
                    isds_strerror(err));
    } else {
        if (!err)
            FAIL_TEST("Message loading should fail: got=%s",
                    isds_strerror(err));
    }

    PASS_TEST;
}


static int test_load_delivery(struct isds_ctx *context,
        const struct test *test) {
    isds_error err;
    int fd;
    void *buffer = NULL;
    size_t length = 0;
    struct isds_message *message = NULL;

    if (!test) return 1;

    if (test_mmap_file(test->file, &fd, &buffer, &length))
        FAIL_TEST("Could not load test data from `%s' file", test->file);

    err = isds_load_delivery_info(context, test->type, buffer, length,
            &message, BUFFER_DONT_STORE);
    isds_message_free(&message);
    test_munmap_file(fd, buffer, length);

    if (test->should_pass) {
        if (err)
            FAIL_TEST("Message loading should succeed: got=%s",
                    isds_strerror(err));
    } else {
        if (!err)
            FAIL_TEST("Message loading should fail: got=%s",
                    isds_strerror(err));
    }

    PASS_TEST;
}


int main(void) {
    struct test messages[] = {
        {
            .name = "unsigned incoming message",
            .file = SRCDIR "/server/messages/received_message-151916.xml",
            .type = RAWTYPE_INCOMING_MESSAGE,
            .should_pass = 1
        },
        {
            .name = "plain signed incoming message",
            .file = SRCDIR
                "/server/messages/received_signed_message-330141.xml",
            .type = RAWTYPE_PLAIN_SIGNED_INCOMING_MESSAGE,
            .should_pass = 1
        },
        {
            .name = "CMS signed incoming message",
            .file = SRCDIR "/server/messages/received_message-330141.zfo",
            .type = RAWTYPE_CMS_SIGNED_INCOMING_MESSAGE,
            .should_pass = 1
        },
        {
            .name = "plain signed sent message",
            .file = SRCDIR "/server/messages/sent_message-206720.xml",
            .type = RAWTYPE_PLAIN_SIGNED_OUTGOING_MESSAGE,
            .should_pass = 1
        },
        {
            .name = "CMS signed sent message",
            .file = SRCDIR "/server/messages/signed_sent_message-151874.zfo",
            .type = RAWTYPE_CMS_SIGNED_OUTGOING_MESSAGE,
            .should_pass = 1
        },
        {
            .name = "plain signed sent message with XML documents",
            .file = SRCDIR "/server/messages/signed_sent_xml_message-376701.xml",
            .type = RAWTYPE_PLAIN_SIGNED_OUTGOING_MESSAGE,
            .should_pass = 1
        },
        {
            .name = "text file is not an incoming message",
            .file = "Makefile",
            .type = RAWTYPE_INCOMING_MESSAGE,
            .should_pass = 0
        },
        {
            .name = NULL
        }
    };

    struct test deliveries[] = {
        {
            .name = "unsigned delivery info",
            .file = SRCDIR "/server/messages/delivery_info-316590.xml",
            .type = RAWTYPE_DELIVERYINFO,
            .should_pass = 1
        },
        {
            .name = "plain signed delivery info",
            .file = SRCDIR "/server/messages/signed_delivered-DD_170272.xml",
            .type = RAWTYPE_PLAIN_SIGNED_DELIVERYINFO,
            .should_pass = 1
        },
        {
            .name = "CMS signed delivery info",
            .file = SRCDIR "/server/messages/signed_delivered-DD_170272.zfo",
            .type = RAWTYPE_CMS_SIGNED_DELIVERYINFO,
            .should_pass = 1
        },
        {
            .name = "text file is not a delivery info",
            .file = "Makefile",
            .type = RAWTYPE_INCOMING_MESSAGE,
            .should_pass = 0
        },
        {
            .name = NULL
        }
    };
    struct isds_ctx *context = NULL;

    INIT_TEST("load_raw");

    if (isds_init())
        ABORT_UNIT("init_isds() failed");

    context = isds_ctx_create();
    if (!context)
        ABORT_UNIT("isds_ctx_create() failed");

    for (int i = 0; messages[i].name; i++)
        TEST(messages[i].name, test_load_message, context, &messages[i]);
    for (int i = 0; deliveries[i].name; i++)
        TEST(deliveries[i].name, test_load_delivery, context, &deliveries[i]);

    isds_ctx_free(&context);
    SUM_TEST();
}
