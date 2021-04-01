#include "../test.h"
#include "crypto.h"
#include <string.h>


struct test {
    char *name;
    char *file;
    isds_raw_type type;
    isds_error error;
};

static int test_guess_raw_type(struct isds_ctx *context,
        const struct test *test) {
    isds_error err;
    int fd;
    void *buffer = NULL;
    size_t length = 0;
    isds_raw_type guessed_type;

    if (!test) return 1;

    if (test_mmap_file(test->file, &fd, &buffer, &length))
        FAIL_TEST("Could not load test data from `%s' file", test->file);

    if (test->error != IE_SUCCESS)
        /* Do not log expected XML parser failures */
        isds_set_logging(ILF_XML, ILL_NONE);
    else
        isds_set_logging(ILF_ALL, ILL_WARNING);

    err = isds_guess_raw_type(context, &guessed_type, buffer, length);
    test_munmap_file(fd, buffer, length);

    if (err != test->error)
        FAIL_TEST("Wrong return value: expected=%s, got=%s",
                isds_strerror(test->error), isds_strerror(err));

    if (!err && guessed_type != test->type)
        FAIL_TEST("Wrong raw type guessed on file %s: expected=%d, got=%d",
               test->file, test->type, guessed_type);

    PASS_TEST;
}


int main(void) {
    struct test tests[] = {
        {
            .name = "unsigned incoming message",
            .file = SRCDIR "/server/messages/received_message-151916.xml",
            .type = RAWTYPE_INCOMING_MESSAGE,
            .error = IE_SUCCESS
        },
        {
            .name = "plain signed incoming message",
            .file = SRCDIR
                "/server/messages/received_signed_message-330141.xml",
            .type = RAWTYPE_PLAIN_SIGNED_INCOMING_MESSAGE,
            .error = IE_SUCCESS
        },
        {
            .name = "CMS signed incoming message",
            .file = SRCDIR "/server/messages/received_message-330141.zfo",
            .type = RAWTYPE_CMS_SIGNED_INCOMING_MESSAGE,
            .error = IE_SUCCESS
        },
        {
            .name = "plain signed sent message",
            .file = SRCDIR "/server/messages/sent_message-206720.xml",
            .type = RAWTYPE_PLAIN_SIGNED_OUTGOING_MESSAGE,
            .error = IE_SUCCESS
        },
        {
            .name = "CMS signed sent message",
            .file = SRCDIR "/server/messages/signed_sent_message-151874.zfo",
            .type = RAWTYPE_CMS_SIGNED_OUTGOING_MESSAGE,
            .error = IE_SUCCESS
        },
        {
            .name = "unsigned delivery info",
            .file = SRCDIR "/server/messages/delivery_info-316590.xml",
            .type = RAWTYPE_DELIVERYINFO,
            .error = IE_SUCCESS
        },
        {
            .name = "plain signed delivery info",
            .file = SRCDIR "/server/messages/signed_delivered-DD_170272.xml",
            .type = RAWTYPE_PLAIN_SIGNED_DELIVERYINFO,
            .error = IE_SUCCESS
        },
        {
            .name = "CMS signed delivery info",
            .file = SRCDIR "/server/messages/signed_delivered-DD_170272.zfo",
            .type = RAWTYPE_CMS_SIGNED_DELIVERYINFO,
            .error = IE_SUCCESS
        },
        {
            .name = "text file",
            .file = "Makefile",
            .error = IE_NOTSUP
        },
        {
            .name = NULL
        }
    };
    struct isds_ctx *context = NULL;

    INIT_TEST("guess_raw_type");

    if (isds_init())
        ABORT_UNIT("init_isds() failed");

    context = isds_ctx_create();
    if (!context)
        ABORT_UNIT("isds_ctx_create() failed");

    for (int i = 0; tests[i].name; i++)
        TEST(tests[i].name, test_guess_raw_type, context, &tests[i]);

    isds_ctx_free(&context);
    SUM_TEST();
}
