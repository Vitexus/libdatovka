#include <libdatovka/isds.h>
#include <locale.h> /* setlocale() */
#include <stdio.h>
#include <stdlib.h>

#include "common.h"

int main(int argc, char **argv)
{
	struct isds_ctx *ctx = NULL;
	enum isds_error err = IE_SUCCESS;
	int ret = EXIT_FAILURE;

	setlocale(LC_ALL, "");

	err = isds_init();
	if (IE_SUCCESS != err) {
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
	if (IE_SUCCESS != err) {
		fprintf(stderr, "isds_set_timeout() failed: %s\n",
		    isds_strerror(err));
	}

	err = isds_login(ctx, url, username(), password(), NULL, NULL);
	if (IE_SUCCESS != err) {
		fprintf(stderr, "isds_login() failed: %s: %s\n",
		    isds_strerror(err), isds_long_message(ctx));
		ret = EXIT_FAILURE;
		goto fail;
	} else {
		fputs("Logged in :)\n", stdout);
	}

	/* Archive (re-sign) ZFO document saved in local file */
	{
		const char *file_path = argv[1];
		int fd;
		void *buffer;
		size_t length;

		void *output_data = NULL;
		size_t output_length = 0;
		struct tm *next_stamp_to = NULL;

		const char *out_from_base = "output_resigned.zfo";

		if (0 != mmap_file(file_path, &fd, &buffer, &length)) {
			fprintf(stderr, "Cannot map file '%s'.\n", file_path);
			ret = EXIT_FAILURE;
			goto fail;
		}
		printf("Attempting to transmit %lu bytes of file '%s'.\n", length, file_path);

		printf("Sending content from file '%s' to ISDS to add a new signature...\n",
		    file_path);

		err = isds_ArchiveISDSDocument(ctx, buffer, length,
		    &output_data, &output_length, &next_stamp_to);
		if (err != IE_SUCCESS) {
			fprintf(stderr, "isds_ArchiveISDSDocument() failed: %s: %s\n",
			    isds_strerror(err), isds_long_message(ctx));
			ret = EXIT_FAILURE;
			goto fail;
		} else {
			printf("isds_ArchiveISDSDocument() succeeded\n");
			save_data_to_file("Saving signed high-volume message",
			    out_from_base, output_data, output_length);
			if (NULL != next_stamp_to) {
				fputs("New signature valid to (-1 day): ", stdout);
				print_date(next_stamp_to);
			}
		}

		free(output_data); output_data = NULL;
		output_length = 0;
		free(next_stamp_to); next_stamp_to = NULL;

	}

fail:
	err = isds_logout(ctx);
	if (IE_SUCCESS != err) {
		fprintf(stderr, "isds_logout() failed: %s\n",
		    isds_strerror(err));
	}

	err = isds_ctx_free(&ctx);
	if (IE_SUCCESS != err) {
		fprintf(stderr, "isds_ctx_free() failed: %s\n",
		    isds_strerror(err));
	}

	err = isds_cleanup();
	if (IE_SUCCESS != err) {
		fprintf(stderr, "isds_cleanup() failed: %s\n",
		    isds_strerror(err));
	}

	exit(ret);
}
