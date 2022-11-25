#define _XOPEN_SOURCE 500
#include <libdatovka/isds.h>
#include <libgen.h> /* basename() */
#include <locale.h> /* setlocale() */
#include <stdlib.h>
#include <string.h> /* strchr() */

#include "common.h"

int main(int argc, char *argv[])
{
	struct isds_ctx *ctx = NULL;
	enum isds_error err = IE_SUCCESS;
	int ret = EXIT_FAILURE;
	struct isds_dmFile dm_file = {0, };
	int fd;
	struct isds_dmAtt *dm_att = NULL;

	setlocale(LC_ALL, "");

	if ((2 != argc) || (NULL == argv[1]) || ('\0' == *argv[1])) {
		fprintf(stderr, "Usage: %s ATTACHMENT_FILE\n",
		    basename(argv[0]));
		exit(EXIT_FAILURE);
	}

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

	err = isds_set_timeout(ctx, 10000);
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
		printf("Logged in :)\n");
	}

	/* Prepare attachment description. */
	{
		const char *file_path = argv[1];
		const char *file_name = basename((char *)file_path);
		const char *suff = strchr(file_name, '.');
		if (NULL != suff) {
			++suff;
		}

		if (0 != mmap_file(file_path, &fd, &dm_file.data, &dm_file.data_length)) {
			fprintf(stderr, "Cannot map file '%s'.\n", file_path);
			ret = EXIT_FAILURE;
			goto fail;
		}

		dm_file.dmMimeType = (char *)isds_normalize_mime_type(suff);
		if (NULL == dm_file.dmMimeType) {
			fprintf(stderr,
			    "Cannot determine MIME type of file '%s'.\n",
			    file_path);
			ret = EXIT_FAILURE;
			goto fail;
		}
		printf("Detected MIME type '%s' for file '%s'.\n",
		    dm_file.dmMimeType, file_path);

		dm_file.dmFileDescr = (char *)file_name;
	}

	/* SOAP/1.1 implementation. */
	err = isds_UploadAttachment(ctx, &dm_file, &dm_att);
	if (err != IE_SUCCESS) {
		fprintf(stderr, "isds_UploadAttachment() failed: %s: %s\n",
		    isds_strerror(err), isds_long_message(ctx));
		ret = EXIT_FAILURE;
		goto fail;
	} else {
		printf("isds_UploadAttachment() succeeded\n");
	}
	{
		const struct isds_status *status = isds_operation_status(ctx);
		if (status != NULL) {
			printf(
			    "Obtained status code: '%s'; message: '%s'; reference number: '%s'\n",
			    status->code, status->message, status->ref_number);
		} else {
			fputs("Cannot obtain status after calling isds_UploadAttachment()\n", stderr);
			ret = EXIT_FAILURE;
			goto fail;
		}
	}
	print_dmAtt(dm_att);

	isds_dmAtt_free(&dm_att);

	/* SOAP/1.2 MTOM/XOP implementation. */
	err = isds_UploadAttachment_mtomxop(ctx, &dm_file, &dm_att);
	if (err != IE_SUCCESS) {
		fprintf(stderr, "isds_UploadAttachment_mtomxop() failed: %s: %s\n",
		    isds_strerror(err), isds_long_message(ctx));
		ret = EXIT_FAILURE;
		goto fail;
	} else {
		printf("isds_UploadAttachment_mtomxop() succeeded\n");
	}
	{
		const struct isds_status *status = isds_operation_status(ctx);
		if (status != NULL) {
			printf(
			    "Obtained status code: '%s'; message: '%s'; reference number: '%s'\n",
			    status->code, status->message, status->ref_number);
		} else {
			fputs("Cannot obtain status after calling isds_UploadAttachment_mtomxop()\n", stderr);
			ret = EXIT_FAILURE;
			goto fail;
		}
	}
	print_dmAtt(dm_att);

	munmap_file(fd, dm_file.data, dm_file.data_length);

fail:
	isds_dmAtt_free(&dm_att);

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
