#define _XOPEN_SOURCE 500
#include <libdatovka/isds.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "common.h"

int main(void)
{
	struct isds_ctx *ctx = NULL;
	enum isds_error err = IE_SUCCESS;

	setlocale(LC_ALL, "");

	err = isds_init();
	if (err != IE_SUCCESS) {
		printf("isds_init() failed: %s\n", isds_strerror(err));
		exit(EXIT_FAILURE);
	}

	isds_set_logging(ILF_ALL & ~ILF_HTTP & ~ILF_SOAP, ILL_ALL);

	ctx = isds_ctx_create();
	if (ctx == NULL) {
		fputs("isds_ctx_create() failed\n", stderr);
	}

	err = isds_set_timeout(ctx, 10000);
	if (err != IE_SUCCESS) {
		fprintf(stderr, "isds_set_timeout() failed: %s\n",
		    isds_strerror(err));
	}

	/* Register progress bar. */
	err = isds_set_progress_callback(ctx, progressbar, NULL);
	if (err != IE_SUCCESS) {
	    printf("isds_set_progress_callback() failed: %s: %s\n",
	            isds_strerror(err), isds_long_message(ctx));
	} else {
	    printf("isds_set_progress_callback() succeeded.\n");
	}

	err = isds_login(ctx, url, username(), password(), NULL, NULL);
	if (err != IE_SUCCESS) {
		fprintf(stderr, "isds_login() failed: %s: %s\n",
		    isds_strerror(err), isds_long_message(ctx));
	} else {
		fputs("Logged in :)\n", stdout);
	}

	/* Register aborting progress bar */
	fputs("\nTesting aborting progress callback\n", stderr);
	err = isds_set_progress_callback(ctx, progressbar, (void *)1);
	if (err != IE_SUCCESS) {
		fprintf(stderr, "isds_set_progress_callback() failed: %s: %s\n",
		    isds_strerror(err), isds_long_message(ctx));
	} else {
		fputs("isds_set_progress_callback() succeeded.\n", stdout);
	}
	err = isds_ping(ctx);
	if (err != IE_SUCCESS) {
		fprintf(stderr, "isds_ping() failed: %s: %s\n",
		    isds_strerror(err), isds_long_message(ctx));
	} else {
		fputs("Ping succeeded\n", stdout);
	}

	/* Register normal progress bar */
	fputs("\nTesting non-aborting progress callback\n", stdout);
	err = isds_set_progress_callback(ctx, progressbar, NULL);
	if (err != IE_SUCCESS) {
		fprintf(stderr, "isds_set_progress_callback() failed: %s: %s\n",
		    isds_strerror(err), isds_long_message(ctx));
	} else {
		fputs("isds_set_progress_callback() succeeded.\n", stdout);
	}
	err = isds_ping(ctx);
	if (err != IE_SUCCESS) {
		fprintf(stderr, "isds_ping() failed: %s: %s\n",
		    isds_strerror(err), isds_long_message(ctx));
	} else {
		fputs("Ping succeeded\n", stderr);
	}

	/* Register no progress bar */
	fputs("\nTesting missing progress callback\n", stdout);
	err = isds_set_progress_callback(ctx, NULL, NULL);
	if (err != IE_SUCCESS) {
		fprintf(stderr, "isds_set_progress_callback() failed: %s: %s\n",
		    isds_strerror(err), isds_long_message(ctx));
	} else {
		fputs("isds_set_progress_callback() succeeded.\n", stdout);
	}
	err = isds_ping(ctx);
	if (err != IE_SUCCESS) {
		fprintf(stderr, "isds_ping() failed: %s: %s\n",
		    isds_strerror(err), isds_long_message(ctx));
	} else {
		fputs("Ping succeeded\n", stderr);
	}

	/* Register aborting progress bar */
	fputs("\nTesting aborting xferinfo callback\n", stderr);
	err = isds_set_xferinfo_callback(ctx, progressbar_int, (void *)1);
	if (err != IE_SUCCESS) {
		fprintf(stderr, "isds_set_xferinfo_callback() failed: %s: %s\n",
		    isds_strerror(err), isds_long_message(ctx));
	} else {
		fputs("isds_set_xferinfo_callback() succeeded.\n", stdout);
	}
	err = isds_ping(ctx);
	if (err != IE_SUCCESS) {
		fprintf(stderr, "isds_ping() failed: %s: %s\n",
		    isds_strerror(err), isds_long_message(ctx));
	} else {
		fputs("Ping succeeded\n", stdout);
	}

	/* Register normal progress bar */
	fputs("\nTesting non-aborting xferinfo callback\n", stdout);
	err = isds_set_xferinfo_callback(ctx, progressbar_int, NULL);
	if (err != IE_SUCCESS) {
		fprintf(stderr, "isds_set_xferinfo_callback() failed: %s: %s\n",
		    isds_strerror(err), isds_long_message(ctx));
	} else {
		fputs("isds_set_xferinfo_callback() succeeded.\n", stdout);
	}
	err = isds_ping(ctx);
	if (err != IE_SUCCESS) {
		fprintf(stderr, "isds_ping() failed: %s: %s\n",
		    isds_strerror(err), isds_long_message(ctx));
	} else {
		fputs("Ping succeeded\n", stderr);
	}

	/* Register no progress bar */
	fputs("\nTesting missing xferinfo callback\n", stdout);
	err = isds_set_xferinfo_callback(ctx, NULL, NULL);
	if (err != IE_SUCCESS) {
		fprintf(stderr, "isds_set_xferinfo_callback() failed: %s: %s\n",
		    isds_strerror(err), isds_long_message(ctx));
	} else {
		fputs("isds_set_xferinfo_callback() succeeded.\n", stdout);
	}
	err = isds_ping(ctx);
	if (err != IE_SUCCESS) {
		fprintf(stderr, "isds_ping() failed: %s: %s\n",
		    isds_strerror(err), isds_long_message(ctx));
	} else {
		fputs("Ping succeeded\n", stderr);
	}


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

	exit(EXIT_SUCCESS);
}
