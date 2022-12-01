#define _XOPEN_SOURCE 500 /* strdup() */
#include <libdatovka/isds.h>
#include <locale.h> /* setlocale() */
#include <stdio.h>
#include <stdlib.h>

#include <string.h> /* strdup() */

#include "common.h"

int main(void)
{
	struct isds_ctx *ctx = NULL;
	enum isds_error err = IE_SUCCESS;
	int ret = EXIT_FAILURE;
	char *last_big_message_id = NULL;
	long int attsNum = 0;

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

	/* Get list of received messages */
	{
		struct tm from_time_tm = {
			.tm_year = 2000 - 1900,
			.tm_mon = 1 - 1,
			.tm_mday = 1,
			.tm_hour = 1,
			.tm_min = 2,
			.tm_sec = 3,
			.tm_isdst = -1
		};
		time_t from_time_t = mktime(&from_time_tm);
		struct isds_timeval from_time = {
			.tv_sec = from_time_t,
			.tv_usec = 4000
		};
		unsigned long int number = 0;
		struct isds_list *messages = NULL;
		const struct isds_list *item;
		const struct isds_message *last_big_message = NULL;

		/* TODO: Try different criteria */
		fputs("Getting list of received messages\n", stdout);
		err = isds_get_list_of_received_messages(ctx, &from_time, NULL, NULL,
		    MESSAGESTATE_ANY, 0, &number, &messages);
		if (err != IE_SUCCESS) {
			printf("isds_get_list_of_received_messages() failed: %s: %s\n",
			    isds_strerror(err), isds_long_message(ctx));
		} else {
			printf(
			    "isds_get_list_of_received_messages() succeeded: number of messages = %lu:\n",
			    number);
			for(item = messages; NULL != item; item = item->next) {
				const struct isds_message *last_message =
				    (struct isds_message *)(item->data);
				if ((NULL != last_message->envelope)
				    && (NULL != last_message->envelope->dmVODZ)
				    && (*last_message->envelope->dmVODZ)) {
					last_big_message = last_message;
				}
			}
		}

		if (NULL != last_big_message) {
			/* Save last message for latter reference */
			if ((NULL != last_big_message->envelope)
			    && (NULL != last_big_message->envelope->dmID)) {
				last_big_message_id = strdup(last_big_message->envelope->dmID);
			}
		}

		isds_list_free(&messages);
	}

	if (NULL == last_big_message_id) {
		fputs("No received high-volume message found.\n", stderr);
		goto fail;
	}

	/* Download envelope to obtain number of attachments. */
	{
		struct isds_message *big_message = NULL;

		printf("Getting envelope of received message '%s'\n", last_big_message_id);
		err = isds_get_received_envelope(ctx, last_big_message_id, &big_message);
		if (err != IE_SUCCESS) {
			printf("isds_get_received_envelope() failed: %s: %s\n",
			    isds_strerror(err), isds_long_message(ctx));
			goto leave1;
		} else {
			fputs("isds_get_received_envelope() succeded\n", stdout);
		}

		if (NULL == big_message) {
			goto leave1;
		}

		if (NULL != big_message->envelope) {
			if ((NULL == big_message->envelope->dmID)
			    || (!*big_message->envelope->dmVODZ)) {
				fputs("Missing true dmVODZ attribute value\n", stderr);
				goto leave1;
			}
			if (NULL != big_message->envelope->attsNum) {
				attsNum = *big_message->envelope->attsNum;
				printf("Message '%s' contains %ld attachments.\n",
				    last_big_message_id, attsNum);
			}
		}

		isds_message_free(&big_message);

		if (0) {
leave1:
			isds_message_free(&big_message);
			goto fail;
		}
	}

	if (attsNum <= 0) {
		fprintf(stderr,
		    "Invalid number of attachments '%ld' for message '%s'.\n",
		    attsNum, last_big_message_id);
		goto fail;
	}

	/* Download attachment. */
	for (int i = 0; i < attsNum; ++i) {
		struct isds_dmFile *dm_file = NULL;

		err = isds_DownloadAttachment(ctx, last_big_message_id, i, &dm_file);
		if (err != IE_SUCCESS) {
			printf("isds_DownloadAttachment() failed: %s: %s\n",
			    isds_strerror(err), isds_long_message(ctx));
			goto leave2;
		} else {
			fputs("isds_DownloadAttachment() suceeded\n", stdout);
			print_dmFile(dm_file);
		}

		isds_dmFile_free(&dm_file);

		if (0) {
leave2:
			isds_dmFile_free(&dm_file);
			goto fail;
		}
	}

fail:
	free(last_big_message_id);

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
