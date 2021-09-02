#define _XOPEN_SOURCE 500

#include <libdatovka/isds.h>
#include <locale.h> /* setlocale */
#include <stdio.h>
#include <stdlib.h>

#include "common.h"

/* Get info about owner. */
static isds_error get_owner(struct isds_ctx *ctx,
    struct isds_DbOwnerInfoExt2 **db_owner_info)
{
	isds_error err = IE_SUCCESS;
	fputs("Getting info about my box:\n", stdout);
	err = isds_GetOwnerInfoFromLogin2(ctx, db_owner_info);
	if (err != IE_SUCCESS) {
		fprintf(stderr, "isds_GetOwnerInfoFromLogin2() failed: %s: %s\n",
		    isds_strerror(err), isds_long_message(ctx));
	} else {
		fputs("isds_GetOwnerInfoFromLogin2() succeeded\n", stdout);
		print_DbOwnerInfoExt2(*db_owner_info);
	}
	return err;
}

int main(void)
{
	struct isds_ctx *ctx = NULL;
	isds_error err;
	struct isds_DbOwnerInfoExt2 *db_owner_info = NULL;
	struct isds_DTInfoOutput *dt_info = NULL;
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
		fprintf(stderr, "isds_ctx_create() failed\n");
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
		fputs("Logged in :)\n", stdout);
	}

	err = get_owner(ctx, &db_owner_info);
	if (err != IE_SUCCESS) {
		ret = EXIT_FAILURE;
		goto fail;
	}

	/* Get information about the long term storage. */
	err = isds_DTInfo(ctx, db_owner_info->dbID, &dt_info);
	if (err != IE_SUCCESS) {
		fprintf(stderr, "isds_DTInfo() failed: %s: %s\n",
		    isds_strerror(err), isds_long_message(ctx));
		print_isds_status(isds_operation_status(ctx));
		goto fail;
	} else {
		fputs("isds_DTInfo() succeeded\n", stdout);
		print_DTInfoOutput(dt_info);
	}

	ret = EXIT_SUCCESS;

fail:
	isds_DTInfoOutput_free(&dt_info);
	isds_DbOwnerInfoExt2_free(&db_owner_info);

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
