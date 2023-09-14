#include <libdatovka/isds.h>
#include <locale.h> /* setlocale() */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h> /* sleep() */

#include "common.h"

static
int pick_up_response_wait(struct isds_ctx *ctx, const char *async_id,
    const char *zip_file_prefix)
{
#define MAX_LEN 1024
	int ret = 0;
	enum isds_error err = IE_SUCCESS;

	void *data = NULL;
	size_t size = 0;

	if ((NULL == ctx) || (NULL == async_id)) {
		ret = -1;
		goto fail;
	}

	err = isds_PickUpAsyncResponse(ctx, async_id,
	    ASYNC_REQ_TYPE_LIST_ERASED, &data, &size);
	while (IE_PARTIAL_SUCCESS == err) {
		printf("Asynchronous response not yet ready\n");
		sleep(5);
		err = isds_PickUpAsyncResponse(ctx, async_id,
		    ASYNC_REQ_TYPE_LIST_ERASED, &data, &size);
	}

	if (err != IE_SUCCESS) {
		fprintf(stderr,
		    "isds_PickUpAsyncResponse() failed: %s: %s\n",
		    isds_strerror(err), isds_long_message(ctx));
		ret = -1;
		goto fail;
	} else {
#define MAX_LEN 1024
		printf("isds_PickUpAsyncResponse() succeeded\n");

		char out_file[MAX_LEN] = {'\0', };
		if ((NULL == zip_file_prefix) || ('\0' == *zip_file_prefix)) {
			zip_file_prefix = "output";
		}

		snprintf(out_file, MAX_LEN, "%s_%s.zip", zip_file_prefix, async_id);

		save_data_to_file("Saving ZIP data", out_file, data, size);
#undef MAX_LEN
	}

fail:
	free(data);

	return ret;
}

int main(void)
{
	struct isds_ctx *ctx = NULL;
	enum isds_error err = IE_SUCCESS;
	int ret = EXIT_FAILURE;

	setlocale(LC_ALL, "");

	err = isds_init();
	if (err != IE_SUCCESS) {
		fprintf(stderr, "isds_init() failed: %s\n", isds_strerror(err));
		ret = EXIT_FAILURE;
		goto fail;
	}

	isds_set_logging(ILF_ALL & ~ILF_HTTP, ILL_ALL);

	ctx = isds_ctx_create();
	if (ctx == NULL) {
		fputs("isds_ctx_create() failed\n", stderr);
		ret = EXIT_FAILURE;
		goto fail;
	}

	/* Increased timeout because of transferring large data portions. */
	err = isds_set_timeout(ctx, 1000000);
	if (err != IE_SUCCESS) {
		fprintf(stderr, "isds_set_timeout() failed: %s\n",
		    isds_strerror(err));
	}

	err = isds_login(ctx, url, username(), password(), NULL, NULL);
	if (err != IE_SUCCESS) {
		fprintf(stderr, "isds_login() failed: %s: %s\n",
		    isds_strerror(err), isds_long_message(ctx));
		ret = EXIT_FAILURE;
		goto fail;
	} else {
		fputs("Logged in :)\n", stdout);
	}

	/* Request list of erased messages for given range. */
	{
		struct tm from_tm = {
			.tm_year = 2023 - 1900,
			.tm_mon = 1 - 1,
			.tm_mday = 1,
			.tm_hour = 0,
			.tm_min = 0,
			.tm_sec = 0,
			.tm_isdst = -1
		};
		struct tm to_tm = {
			.tm_year = 2023 - 1900,
			.tm_mon = 2 - 1,
			.tm_mday = 1,
			.tm_hour = 0,
			.tm_min = 0,
			.tm_sec = 0,
			.tm_isdst = -1
		};

		{
			char *async_id = NULL;

			fputs("Getting list of erased messages\n", stdout);
			err = isds_GetListOfErasedMessages_interval(ctx,
			    &from_tm, &to_tm, MESSAGE_TYPE_SENT, FORMAT_XML, &async_id);
			if (err != IE_SUCCESS) {
				fprintf(stderr,
				    "isds_GetListOfErasedMessages_interval() failed: %s: %s\n",
				    isds_strerror(err), isds_long_message(ctx));
			} else {
				printf(
				    "isds_GetListOfErasedMessages_interval() succeeded: async_id = %s\n",
				    async_id);
			}

			if ((NULL != async_id) && ('\0' != async_id[0])) {
				pick_up_response_wait(ctx, async_id, "output_sent_xml");
			}

			free(async_id); async_id = NULL;
		}

		{
			char *async_id = NULL;

			fputs("Getting list of erased messages\n", stdout);
			err = isds_GetListOfErasedMessages_interval(ctx,
			    &from_tm, &to_tm, MESSAGE_TYPE_RECEIVED, FORMAT_XML, &async_id);
			if (err != IE_SUCCESS) {
				fprintf(stderr,
				    "isds_GetListOfErasedMessages_interval() failed: %s: %s\n",
				    isds_strerror(err), isds_long_message(ctx));
			} else {
				printf(
				    "isds_GetListOfErasedMessages_interval() succeeded: async_id = %s\n",
				    async_id);
			}

			if ((NULL != async_id) && ('\0' != async_id[0])) {
				pick_up_response_wait(ctx, async_id, "output_received_xml");
			}

			free(async_id); async_id = NULL;
		}
	}

	/* Request list of erased messages for given month. */
	{
		unsigned int year = 2023;
		unsigned int month = 1;

		{
			char *async_id = NULL;

			fputs("Getting list of erased messages\n", stdout);
			err = isds_GetListOfErasedMessages_month(ctx,
			    year, month, MESSAGE_TYPE_SENT, FORMAT_CSV, &async_id);
			if (err != IE_SUCCESS) {
				fprintf(stderr,
				    "isds_GetListOfErasedMessages_month() failed: %s: %s\n",
				    isds_strerror(err), isds_long_message(ctx));
			} else {
				printf(
				    "isds_GetListOfErasedMessages_month() succeeded: async_id = %s\n",
				    async_id);
			}

			if ((NULL != async_id) && ('\0' != async_id[0])) {
				pick_up_response_wait(ctx, async_id, "output_sent_csv");
			}

			free(async_id); async_id = NULL;
		}

		{
			char *async_id = NULL;

			fputs("Getting list of erased messages\n", stdout);
			err = isds_GetListOfErasedMessages_month(ctx,
			    year, month, MESSAGE_TYPE_RECEIVED, FORMAT_CSV, &async_id);
			if (err != IE_SUCCESS) {
				fprintf(stderr,
				    "isds_GetListOfErasedMessages_month() failed: %s: %s\n",
				    isds_strerror(err), isds_long_message(ctx));
			} else {
				printf(
				    "isds_GetListOfErasedMessages_month() succeeded: async_id = %s\n",
				    async_id);
			}

			if ((NULL != async_id) && ('\0' != async_id[0])) {
				pick_up_response_wait(ctx, async_id, "output_received_csv");
			}

			free(async_id); async_id = NULL;
		}
	}

	/* Request list of erased messages for given year. */
	{
		unsigned int year = 2022;

		{
			char *async_id = NULL;

			fputs("Getting list of erased messages\n", stdout);
			err = isds_GetListOfErasedMessages_year(ctx,
			    year, MESSAGE_TYPE_SENT, FORMAT_XML, &async_id);
			if (err != IE_SUCCESS) {
				fprintf(stderr,
				    "isds_GetListOfErasedMessages_year() failed: %s: %s\n",
				    isds_strerror(err), isds_long_message(ctx));
			} else {
				printf(
				    "isds_GetListOfErasedMessages_year() succeeded: async_id = %s\n",
				    async_id);
			}

			if ((NULL != async_id) && ('\0' != async_id[0])) {
				pick_up_response_wait(ctx, async_id, "output_sent_xml");
			}

			free(async_id); async_id = NULL;
		}

		{
			char *async_id = NULL;

			fputs("Getting list of erased messages\n", stdout);
			err = isds_GetListOfErasedMessages_year(ctx,
			    year, MESSAGE_TYPE_RECEIVED, FORMAT_XML, &async_id);
			if (err != IE_SUCCESS) {
				fprintf(stderr,
				    "isds_GetListOfErasedMessages_year() failed: %s: %s\n",
				    isds_strerror(err), isds_long_message(ctx));
			} else {
				printf(
				    "isds_GetListOfErasedMessages_year() succeeded: async_id = %s\n",
				    async_id);
			}

			if ((NULL != async_id) && ('\0' != async_id[0])) {
				pick_up_response_wait(ctx, async_id, "output_received_xml");
			}

			free(async_id); async_id = NULL;
		}

		{
			char *async_id = NULL;

			fputs("Getting list of erased messages\n", stdout);
			err = isds_GetListOfErasedMessages_year(ctx,
			    year, MESSAGE_TYPE_SENT, FORMAT_CSV, &async_id);
			if (err != IE_SUCCESS) {
				fprintf(stderr,
				    "isds_GetListOfErasedMessages_year() failed: %s: %s\n",
				    isds_strerror(err), isds_long_message(ctx));
			} else {
				printf(
				    "isds_GetListOfErasedMessages_year() succeeded: async_id = %s\n",
				    async_id);
			}

			if ((NULL != async_id) && ('\0' != async_id[0])) {
				pick_up_response_wait(ctx, async_id, "output_sent_csv");
			}

			free(async_id); async_id = NULL;
		}

		{
			char *async_id = NULL;

			fputs("Getting list of erased messages\n", stdout);
			err = isds_GetListOfErasedMessages_year(ctx,
			    year, MESSAGE_TYPE_RECEIVED, FORMAT_CSV, &async_id);
			if (err != IE_SUCCESS) {
				fprintf(stderr,
				    "isds_GetListOfErasedMessages_year() failed: %s: %s\n",
				    isds_strerror(err), isds_long_message(ctx));
			} else {
				printf(
				    "isds_GetListOfErasedMessages_year() succeeded: async_id = %s\n",
				    async_id);
			}

			if ((NULL != async_id) && ('\0' != async_id[0])) {
				pick_up_response_wait(ctx, async_id, "output_received_csv");
			}

			free(async_id); async_id = NULL;
		}
	}

	/* Try some identifiers. */
	{
		const char *async_ids[] = {
		    "10120", "10121", "10122", "10123", "10124",
		    "10125", "10126", "10127", "10128", "10129",
		    "10130", "10131", "10132", "10133", "10134",
		    "10135", "10136", "10137", "10138", "10139",
		    NULL};

		for (const char **aid = async_ids; NULL != *aid; ++aid) {
			pick_up_response_wait(ctx, *aid, "output");
		}
	}

fail:
	err = isds_logout(ctx);
	if (err != IE_SUCCESS) {
		fprintf(stderr, "isds_logout() failed: %s\n",
		    isds_strerror(err));
	}

	err = isds_ctx_free(&ctx);
	if (err != IE_SUCCESS) {
		fprintf(stderr, "isds_ctx_free() failed: %s\n",
		    isds_strerror(err));
	}

	err = isds_cleanup();
	if (err != IE_SUCCESS) {
		fprintf(stderr, "isds_cleanup() failed: %s\n",
		    isds_strerror(err));
	}

	exit(ret);
}
