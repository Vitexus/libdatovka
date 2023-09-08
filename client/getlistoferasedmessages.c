#include <libdatovka/isds.h>
#include <locale.h> /* setlocale() */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "common.h"

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

	/* Request list of erased messages. */
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
		char *async_id = NULL;

		fputs("Getting list of erased messages\n", stdout);
		err = isds_GetListOfErasedMessages_interval(ctx,
		    &from_tm, &to_tm, MESSAGE_TYPE_SENT, OUT_XML, &async_id);

		if (err != IE_SUCCESS) {
			printf("isds_GetListOfErasedMessages_interval() failed: %s: %s\n",
			    isds_strerror(err), isds_long_message(ctx));
		} else {
			printf(
			    "isds_GetListOfErasedMessages_interval() succeeded: async_id = %s\n",
			    async_id);
		}

		free(async_id); async_id = NULL;
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
