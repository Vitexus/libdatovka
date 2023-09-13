#include "../test.h"
#include "crypto.h"
#include <string.h>
#include <time.h> /* localtime_r() */

//#define PRINT_DATA 1

struct test {
	char *name;
	char *file;
	enum isds_dmOutFormat format;
	enum isds_error error;
};

#ifdef PRINT_DATA
static
void print_dmMessageStatus(const isds_message_status *status) {
	if (NULL == status) {
		fputs("NULL\n", stdout);
	} else {
		switch(*status) {
		case MESSAGESTATE_SENT: fputs("SENT\n", stdout); break;
		case MESSAGESTATE_STAMPED: fputs("STAMPED\n", stdout); break;
		case MESSAGESTATE_INFECTED: fputs("INFECTED\n", stdout); break;
		case MESSAGESTATE_DELIVERED: fputs("DELIVERED\n", stdout); break;
		case MESSAGESTATE_SUBSTITUTED: fputs("SUBSTITUTED\n", stdout); break;
		case MESSAGESTATE_RECEIVED: fputs("RECEIVED\n", stdout); break;
		case MESSAGESTATE_READ: fputs("READ\n", stdout); break;
		case MESSAGESTATE_UNDELIVERABLE: fputs("UNDELIVERABLE\n", stdout); break;
		case MESSAGESTATE_REMOVED: fputs("REMOVED\n", stdout); break;
		case MESSAGESTATE_IN_VAULT: fputs("IN_VAULT\n", stdout); break;
		default: fprintf(stdout, "<unknown type %d>\n", (int)*status);
		}
	}
}

static
void print_timeval(const struct isds_timeval *time) { // !!!
	struct tm broken;
	char buffer[128];
	time_t seconds_as_time_t;

	if (NULL == time) {
		fputs("NULL\n", stdout);
		return;
	}

	/*
	 * MinGW32 GCC 4.8+ uses 64-bit time_t but time->tv_sec is defined as
	 * 32-bit long in Microsoft API. Convert value to the type expected by
	 * gmtime_r().
	 */
	seconds_as_time_t = time->tv_sec;
	if (NULL == localtime_r(&seconds_as_time_t, &broken)) {
		goto error;
	}
	if (0 == strftime(buffer, sizeof(buffer)/sizeof(char), "%c", &broken)) {
		goto error;
	}
	fprintf(stdout, "%s, %" PRIdMAX " us\n", buffer, (intmax_t)time->tv_usec);
	return;

error:
	fputs("<Error while formatting>\n>", stdout);
	return;
}

static
void print_erased_message(const struct isds_erased_message *erased_message)
{
	fputs("\erased_message = ", stdout);

	if (NULL == erased_message) {
		fputs("NULL\n", stdout);
		return;
	}
	fputs("{\n", stdout);

	fprintf(stdout, "\t\tdmID = %s\n", erased_message->dmID);
	fprintf(stdout, "\t\tdbIDSender = %s\n", erased_message->dbIDSender);
	fprintf(stdout, "\t\tdmSender = %s\n", erased_message->dmSender);
	fprintf(stdout, "\t\tdbIDRecipient = %s\n", erased_message->dbIDRecipient);
	fprintf(stdout, "\t\tdmRecipient = %s\n", erased_message->dmRecipient);
	fprintf(stdout, "\t\tdmAnnotation = %s\n", erased_message->dmAnnotation);

	fputs("\t\tdmMessageStatus = ", stdout);
	print_dmMessageStatus(erased_message->dmMessageStatus);

	fputs("\t\tdmDeliveryTime = ", stdout);
	print_timeval(erased_message->dmDeliveryTime);

	fputs("\t\tdmAcceptanceTime = ", stdout);
	print_timeval(erased_message->dmAcceptanceTime);

	fprintf(stdout, "\t\tdmType = %s\n", erased_message->dmType);

	fputs("\t}\n", stdout);
}
#endif /* PRINT_DATA */

static
int test_load_erased_messages(struct isds_ctx *context,
    const struct test *test)
{
	enum isds_error err;
	int fd;
	void *buffer = NULL;
	size_t length = 0;
	struct isds_list *erased_messages = NULL;

	if (NULL == test) {
		return 1;
	}

	if (0 != test_mmap_file(test->file, &fd, &buffer, &length)) {
		FAIL_TEST("Could not load test data from `%s' file", test->file);
	}

	if (test->error != IE_SUCCESS) {
		/* Do not log expected XML parser failures */
		isds_set_logging(ILF_XML, ILL_NONE);
	} else {
		isds_set_logging(ILF_ALL, ILL_WARNING);
	}

	err = isds_load_erased_messages(context, test->format, buffer, length,
	    &erased_messages);

	test_munmap_file(fd, buffer, length);

	if (err != test->error) {
		FAIL_TEST("Wrong return value: expected=%s, got=%s",
		    isds_strerror(test->error), isds_strerror(err));
	}

#ifdef PRINT_DATA
	for (const struct isds_list *item = erased_messages; NULL != item; item = item->next) {
		print_erased_message((const struct isds_erased_message *)item->data);
	}
#endif /* PRINT_DATA */

	isds_list_free(&erased_messages);

	PASS_TEST;
}

int main(void) {
	struct test tests[] = {
		{
			.name = "decompressed XML response",
			.file = SRCDIR "/test/offline/data/GetListOfErasedMessages-async-response-extracted.xml",
			.format = OUT_XML,
			.error = IE_SUCCESS
		},
		{
			.name = "decompressed XML response with missing values",
			.file = SRCDIR "/test/offline/data/GetListOfErasedMessages-missing-entries.xml",
			.format = OUT_XML,
			.error = IE_SUCCESS
		},
		{
			.name = NULL
		}
	};

	struct isds_ctx *context = NULL;

	INIT_TEST("isds_load_erased_messages");

	if (IE_SUCCESS != isds_init()) {
		ABORT_UNIT("init_isds() failed");
	}

	context = isds_ctx_create();
	if (NULL == context) {
		ABORT_UNIT("isds_ctx_create() failed");
	}

	for (int i = 0; tests[i].name; ++i) {
		TEST(tests[i].name, test_load_erased_messages, context, &tests[i]);
	}

	isds_ctx_free(&context);
	SUM_TEST();
}
