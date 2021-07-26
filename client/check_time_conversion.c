
#define _XOPEN_SOURCE 500

#include <libdatovka/isds.h>
#include <locale.h> /* setlocale */
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	isds_error err = IE_ERROR;
	struct isds_ctx *ctx = NULL;

	setlocale(LC_ALL, "");

	err = isds_init();
	if (err != IE_SUCCESS) {
		fprintf(stderr, "isds_init() failed: %s\n", isds_strerror(err));
		return EXIT_FAILURE;
	}

	ctx = isds_ctx_create();
	if (ctx == NULL) {
		fprintf(stderr, "isds_ctx_create() failed\n");
		return EXIT_FAILURE;
	}

	err = isds_check_func_timegm(ctx);
	if (err != IE_SUCCESS) {
		fprintf(stderr, "isds_check_func_timegm() failed: %s\n",
		    isds_long_message(ctx));
	}

	err = isds_check_func_gmtime_r(ctx);
	if (err != IE_SUCCESS) {
		fprintf(stderr, "isds_check_func_gmtime_r() failed: %s\n",
		    isds_long_message(ctx));
	}

	err = isds_ctx_free(&ctx);
	if (err != IE_SUCCESS) {
		fprintf(stderr, "isds_ctx_free() failed: %s\n", isds_strerror(err));
	}

	err = isds_cleanup();
	if (err != IE_SUCCESS) {
		fprintf(stderr, "isds_cleanup() failed: %s\n", isds_strerror(err));
	}

	return EXIT_SUCCESS;
}
