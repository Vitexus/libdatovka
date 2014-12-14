#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE   /* For getaddrinfo(3) */
#endif

#ifndef _BSD_SOURCE
#define _BSD_SOURCE   /* For NI_MAXHOST up to glibc-2.19 */
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE   /* For NI_MAXHOST since glibc-2.20 */
#endif

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600   /* For unsetenv(3) */
#endif

#include "../test.h"
#include "server.h"
#include "isds.h"

static const char *username = "Doug1as$";
static const char *password = "42aA#bc8";


static int test_login(const isds_error error, struct isds_ctx *context,
        const char *url, const char *username, const char *password,
        const struct isds_pki_credentials *pki_credentials,
        struct isds_otp *otp) {
    isds_error err;

    err = isds_login(context, url, username, password, pki_credentials, otp);
    if (error != err)
        FAIL_TEST("Wrong return code: expected=%s, returned=%s (%s)",
                isds_strerror(error), isds_strerror(err),
                isds_long_message(context));

    PASS_TEST;
}

struct test_destructor_argument {
    char *returned_email;
    struct isds_list *returned_history;
};

static void test_destructor(void *argument) {
    if (NULL == argument) return;
    free(((struct test_destructor_argument *)argument)->returned_email); 
    isds_list_free(
            &((struct test_destructor_argument *)argument)->returned_history
    ); 
}

static int test_isds_get_commercial_credit(const isds_error error,
        struct isds_ctx *context, const char *box_id,
        const struct tm *from_date, const struct tm *to_date,
        const long int *credit, const char **email,
        const struct isds_list *history) {
    isds_error returned_error;
    long int returned_credit = -1;

    struct test_destructor_argument allocated = {
        .returned_email = strdup("foo"),
        .returned_history = NULL
    };

    const struct isds_list *item;
    struct isds_list *returned_item;
    const struct isds_credit_event *event;
    struct isds_credit_event *returned_event;

    returned_error = isds_get_commercial_credit(context, box_id,
            from_date, to_date,
            (NULL == credit) ? NULL : &returned_credit,
            (NULL == email) ? NULL : &allocated.returned_email,
            (NULL == history) ? NULL : &allocated.returned_history);
    TEST_DESTRUCTOR(test_destructor, &allocated);

    if (error != returned_error) {
        FAIL_TEST("Wrong return code: expected=%s, returned=%s (%s)",
                isds_strerror(error), isds_strerror(returned_error),
                isds_long_message(context));
    }

    if (IE_SUCCESS != returned_error) {
        if (NULL != email && NULL != allocated.returned_email)
            FAIL_TEST("email is not NULL on error");
        if (NULL != history && NULL != allocated.returned_history)
            FAIL_TEST("history is not NULL on error");
        PASS_TEST;
    }

    if (NULL != email)
        TEST_STRING_DUPLICITY(*email, allocated.returned_email);
    if (NULL != credit)
        TEST_INT_DUPLICITY(*credit, returned_credit);
    for (item = history, returned_item = allocated.returned_history;
            NULL != item;
            item = item->next, returned_item = returned_item->next) {
        if (NULL == returned_item)
            FAIL_TEST("Returned history has too few items");
        event = (struct isds_credit_event *)item->data;
        returned_event = (struct isds_credit_event *)returned_item->data;
        if (NULL == event) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT("History event is not defined");
        }
        if (NULL == returned_item)
            FAIL_TEST("Returned history event is NULL and it shouldn't be");
        TEST_TIMEVALPTR_DUPLICITY(event->time, returned_event->time);
        TEST_INT_DUPLICITY(event->credit_change, returned_event->credit_change);
        TEST_INT_DUPLICITY(event->new_credit, returned_event->new_credit);
        TEST_INT_DUPLICITY(event->type, returned_event->type);

        switch (event->type) {
            case ISDS_CREDIT_CHARGED:
                TEST_STRING_DUPLICITY(event->details.charged.transaction,
                        returned_event->details.charged.transaction);
                break;
            case ISDS_CREDIT_DISCHARGED:
                TEST_STRING_DUPLICITY(event->details.discharged.transaction,
                        returned_event->details.discharged.transaction);
                break;
            case ISDS_CREDIT_MESSAGE_SENT:
                TEST_STRING_DUPLICITY(event->details.message_sent.recipient,
                        returned_event->details.message_sent.recipient);
                TEST_STRING_DUPLICITY(event->details.message_sent.message_id,
                        returned_event->details.message_sent.message_id);
                break;
            case ISDS_CREDIT_STORAGE_SET:
                TEST_INT_DUPLICITY(event->details.storage_set.new_capacity,
                        returned_event->details.storage_set.new_capacity);
                TEST_TMPTR_DUPLICITY(event->details.storage_set.new_valid_from,
                        returned_event->details.storage_set.new_valid_from);
                TEST_TMPTR_DUPLICITY(event->details.storage_set.new_valid_to,
                        returned_event->details.storage_set.new_valid_to);
                TEST_INTPTR_DUPLICITY(event->details.storage_set.old_capacity,
                        returned_event->details.storage_set.old_capacity);
                TEST_TMPTR_DUPLICITY(event->details.storage_set.old_valid_from,
                        returned_event->details.storage_set.old_valid_from);
                TEST_TMPTR_DUPLICITY(event->details.storage_set.old_valid_to,
                        returned_event->details.storage_set.old_valid_to);
                TEST_STRING_DUPLICITY(event->details.storage_set.initiator,
                        returned_event->details.storage_set.initiator);
                break;
            case ISDS_CREDIT_EXPIRED:
                break;
            default:
                FAIL_TEST("Uknown credit event type returned");
        }
    }
    if (NULL != returned_item)
        FAIL_TEST("Returned history has too many items");

    
    PASS_TEST;
}

int main(int argc, char **argv) {
    int error;
    pid_t server_process;
    struct isds_ctx *context = NULL;

    INIT_TEST("isds_get_commercial_credit");

    if (unsetenv("http_proxy")) {
        ABORT_UNIT("Could not remove http_proxy variable from environment\n");
    }
    if (isds_init()) {
        isds_cleanup();
        ABORT_UNIT("isds_init() failed\n");
    }
    context = isds_ctx_create();
    if (!context) {
        isds_cleanup();
        ABORT_UNIT("isds_ctx_create() failed\n");
    }

    {
        char *url = NULL;
        const char *box_id = "abcefgh";
        const struct tm from_date = {
            .tm_year = 1,
            .tm_mon = 1,
            .tm_mday = 3
        };
        const struct tm to_date = {
            .tm_year = 4,
            .tm_mon = 4,
            .tm_mday = 6
        };
        struct tm new_valid_from = {
            .tm_year = 2,
            .tm_mon = 2,
            .tm_mday = 7
        };
        struct tm new_valid_to = {
            .tm_year = 3,
            .tm_mon = 3,
            .tm_mday = 8
        };
        struct tm old_valid_from = {
            .tm_year = 5,
            .tm_mon = 5,
            .tm_mday = 9
        };
        struct tm old_valid_to = {
            .tm_year = 6,
            .tm_mon = 6,
            .tm_mday = 10
        };
        const long int credit = 42;
        long int old_capacity = 43;
        const char *email = "joe@example.com";
        struct timeval event_time = {
            .tv_sec = 981173106,
            .tv_usec = 123456
        };

        struct isds_credit_event event_credit_expired = {
            .time = &event_time,
            .credit_change = 133,
            .new_credit = 244,
            .type = ISDS_CREDIT_EXPIRED,
        };
        struct server_credit_event server_event_credit_expired = {
            .time = &event_time,
            .credit_change = 133,
            .new_credit = 244,
            .type = SERVER_CREDIT_EXPIRED,
        };
        struct isds_list history_expired = {
            .next = NULL,
            .data = &event_credit_expired,
            .destructor = NULL
        };
        struct server_list server_history_expired = {
            .next = NULL,
            .data = &server_event_credit_expired,
            .destructor = NULL
        };

        struct isds_credit_event event_credit_storage_set = {
            .time = &event_time,
            .credit_change = 133,
            .new_credit = 244,
            .type = ISDS_CREDIT_STORAGE_SET,
            .details.storage_set.new_capacity = 41,
            .details.storage_set.new_valid_from = &new_valid_from,
            .details.storage_set.new_valid_to = &new_valid_to,
            .details.storage_set.old_capacity = &old_capacity,
            .details.storage_set.old_valid_from = &old_valid_from,
            .details.storage_set.old_valid_to = &old_valid_to,
            .details.storage_set.initiator = "Foo",
        };
        struct server_credit_event server_event_credit_storage_set = {
            .time = &event_time,
            .credit_change = 133,
            .new_credit = 244,
            .type = SERVER_CREDIT_STORAGE_SET,
            .details.storage_set.new_capacity = 41,
            .details.storage_set.new_valid_from = &new_valid_from,
            .details.storage_set.new_valid_to = &new_valid_to,
            .details.storage_set.old_capacity = &old_capacity,
            .details.storage_set.old_valid_from = &old_valid_from,
            .details.storage_set.old_valid_to = &old_valid_to,
            .details.storage_set.initiator = "Foo",
        };
        struct isds_list history_storage_set = {
            .next = &history_expired,
            .data = &event_credit_storage_set,
            .destructor = NULL
        };
        struct server_list server_history_storage_set = {
            .next = &server_history_expired,
            .data = &server_event_credit_storage_set,
            .destructor = NULL
        };

        struct isds_credit_event event_credit_message_sent = {
            .time = &event_time,
            .credit_change = 133,
            .new_credit = 244,
            .type = ISDS_CREDIT_MESSAGE_SENT,
            .details.message_sent.recipient = "Foo",
            .details.message_sent.message_id = "ijklmnop",
        };
        struct server_credit_event server_event_credit_message_sent = {
            .time = &event_time,
            .credit_change = 133,
            .new_credit = 244,
            .type = SERVER_CREDIT_MESSAGE_SENT,
            .details.message_sent.recipient = "Foo",
            .details.message_sent.message_id = "ijklmnop",
        };
        struct isds_list history_message_sent = {
            .next = &history_storage_set,
            .data = &event_credit_message_sent,
            .destructor = NULL
        };
        struct server_list server_history_message_sent = {
            .next = &server_history_storage_set,
            .data = &server_event_credit_message_sent,
            .destructor = NULL
        };

        struct isds_credit_event event_credit_discharged = {
            .time = &event_time,
            .credit_change = 133,
            .new_credit = 244,
            .type = ISDS_CREDIT_DISCHARGED,
            .details.discharged.transaction = "Foo"
        };
        struct server_credit_event server_event_credit_discharged = {
            .time = &event_time,
            .credit_change = 133,
            .new_credit = 244,
            .type = SERVER_CREDIT_DISCHARGED,
            .details.discharged.transaction = "Foo"
        };
        struct isds_list history_discharged = {
            .next = &history_message_sent,
            .data = &event_credit_discharged,
            .destructor = NULL
        };
        struct server_list server_history_discharged = {
            .next = &server_history_message_sent,
            .data = &server_event_credit_discharged,
            .destructor = NULL
        };

        struct isds_credit_event event_credit_charged = {
            .time = &event_time,
            .credit_change = 133,
            .new_credit = 244,
            .type = ISDS_CREDIT_CHARGED,
            .details.charged.transaction = "Foo"
        };
        struct server_credit_event server_event_credit_charged = {
            .time = &event_time,
            .credit_change = 133,
            .new_credit = 244,
            .type = SERVER_CREDIT_CHARGED,
            .details.charged.transaction = "Foo"
        };
        const struct isds_list history = {
            .next = &history_discharged,
            .data = &event_credit_charged,
            .destructor = NULL
        };
        const struct server_list server_history = {
            .next = &server_history_discharged,
            .data = &server_event_credit_charged,
            .destructor = NULL
        };


        const struct arguments_DS_df_DataBoxCreditInfo service_arguments = {
            .status_code = "0000",
            .status_message = "Ok.",
            .box_id = box_id,
            .from_date = &from_date,
            .to_date = &to_date,
            .current_credit = credit,
            .email = email,
            .history = &server_history
        };
        const struct service_configuration services[] = {
            { SERVICE_DS_Dz_DummyOperation, NULL },
            { SERVICE_DS_df_DataBoxCreditInfo, &service_arguments },
            { SERVICE_END, NULL }
        };
        const struct arguments_basic_authentication server_arguments = {
            .username = username,
            .password = password,
            .isds_deviations = 1,
            .services = services
        };
        error = start_server(&server_process, &url,
                server_basic_authentication, &server_arguments, NULL);
        if (error == -1) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT(server_error);
        }
        TEST("login", test_login, IE_SUCCESS,
                context, url, username, password, NULL, NULL);
        free(url);

        TEST("NULL box_id", test_isds_get_commercial_credit, IE_INVAL,
                context, NULL, NULL, NULL, NULL, NULL, NULL);
        TEST("All data", test_isds_get_commercial_credit, IE_SUCCESS,
                context, box_id, &from_date, &to_date, &credit, &email,
                &history);
        TEST("Wrong box_id", test_isds_get_commercial_credit, IE_ISDS,
                context, "1", &from_date, &to_date, &credit, &email,
                &history);
        TEST("No history", test_isds_get_commercial_credit, IE_SUCCESS,
                context, box_id, &from_date, &to_date, &credit, &email, NULL);

        isds_logout(context);
        if (stop_server(server_process)) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT(server_error);
        }
    }


    {
        char *url = NULL;
        const char *box_id = "abcefgh";
        const long int credit = 42;
        const char *email = "joe@example.com";

        const struct arguments_DS_df_DataBoxCreditInfo service_arguments = {
            .status_code = "0000",
            .status_message = "Ok.",
            .box_id = box_id,
            .from_date = NULL,
            .to_date = NULL,
            .current_credit = credit,
            .email = email,
            .history = NULL
        };
        const struct service_configuration services[] = {
            { SERVICE_DS_Dz_DummyOperation, NULL },
            { SERVICE_DS_df_DataBoxCreditInfo, &service_arguments },
            { SERVICE_END, NULL }
        };
        const struct arguments_basic_authentication server_arguments = {
            .username = username,
            .password = password,
            .isds_deviations = 1,
            .services = services
        };
        error = start_server(&server_process, &url,
                server_basic_authentication, &server_arguments, NULL);
        if (error == -1) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT(server_error);
        }
        TEST("login", test_login, IE_SUCCESS,
                context, url, username, password, NULL, NULL);
        free(url);

        TEST("No dates", test_isds_get_commercial_credit, IE_SUCCESS,
                context, box_id, NULL, NULL, &credit, &email, NULL);

        isds_logout(context);
        if (stop_server(server_process)) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT(server_error);
        }
    }


    isds_ctx_free(&context);
    isds_cleanup();
    SUM_TEST();
}
