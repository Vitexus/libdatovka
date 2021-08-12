
#define _XOPEN_SOURCE 500

#include <libdatovka/isds.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"

/* Get info about owner. */
static isds_error get_owner(struct isds_ctx *ctx,
    struct isds_DbOwnerInfoExt2 **db_owner_info)
{
	isds_error err = IE_SUCCESS;
	printf("Getting info about my box:\n");
	err = isds_GetOwnerInfoFromLogin2(ctx, db_owner_info);
	if (err != IE_SUCCESS) {
		fprintf(stderr, "isds_GetOwnerInfoFromLogin2() failed: %s: %s\n",
		    isds_strerror(err), isds_long_message(ctx));
	} else {
		printf("isds_GetOwnerInfoFromLogin2() succeeded\n");
		print_DbOwnerInfoExt2(*db_owner_info);
	}
	return err;
}

static void print_isds_payment_type(const long int type)
{
	switch (type) {
	case PAYMENT_SENDER: fputs("SENDER\n", stdout); break;
	case PAYMENT_STAMP: fputs("STAMP\n", stdout); break;
	case PAYMENT_SPONSOR: fputs("SPONSOR\n", stdout); break;
	case PAYMENT_RESPONSE: fputs("RESPONSE\n", stdout); break;
	case PAYMENT_SPONSOR_LIMITED: fputs("SPONSOR_LIMITED\n", stdout); break;
	case PAYMENT_SPONSOR_EXTERNAL: fputs("SPONSOR_EXTERNAL\n", stdout); break;
	default: printf("<unknown type %ld>\n", type); break;
	}
}

static void print_isds_commercial_permissions(
    const struct isds_list *permissions)
{
	const struct isds_list *item;
	const struct isds_commercial_permission *permission;

	fputs("PDZ Records = ", stdout);
	if (permissions == NULL) {
		fputs("<NULL>\n", stdout);
		return;
	}

	fputs("{\n", stdout);
	for (item = permissions; item != NULL; item = item->next) {
		permission = (const struct isds_commercial_permission *)item->data;

		fputs("\tPDZ Record = ", stdout);
		if (permission == NULL) {
			fputs("\t<NULL>\n", stdout);
			return;
		}

		fputs("{\n", stdout);
		printf("\t\ttype = ");
		print_isds_payment_type(permission->type);
		printf("\t\trecipient = %s\n", permission->recipient);
		printf("\t\tpayer = %s\n", permission->payer);
		printf("\t\texpiration = ");
		print_timeval(permission->expiration);
		printf("\t\tcount = ");
		if (permission->count == NULL) {
			fputs("NULL\n", stdout);
		} else {
			printf("%ld\n", *(permission->count));
		}
		printf("\t\treply_identifier = %s\n",
		    permission->reply_identifier);
		fputs("\t}\n", stdout);
	}
	fputs("}\n", stdout);
}

int main(void)
{
	struct isds_ctx *ctx = NULL;
	isds_error err;
	struct isds_DbOwnerInfoExt2 *db_owner_info = NULL;
	struct isds_list *permissions = NULL;
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
		printf("Logged in :)\n");
	}

	err = get_owner(ctx, &db_owner_info);
	if (err != IE_SUCCESS) {
		ret = EXIT_FAILURE;
		goto fail;
	}

	{
		err = isds_get_commercial_permissions(ctx, db_owner_info->dbID,
		    &permissions);
		if (err == IE_SUCCESS) {
			fputs("isds_get_commercial_permissions() succeeded\n",
			    stdout);
			print_isds_commercial_permissions(permissions);
		} else {
			fprintf(stderr,
			    "isds_get_commercial_permissions() failed: %s: %s\n",
			    isds_strerror(err), isds_long_message(ctx));
		}
	}

	{
		_Bool can_send = true;
		err = isds_PDZSendInfo(ctx, db_owner_info->dbID,
		    COMMERCIAL_NORMAL, &can_send);
		if (err == IE_SUCCESS) {
			fputs("isds_PDZSendInfo() succeeded\n", stdout);
			printf(
			    "User %s send normal commercial messages to recipient %s.\n",
			    can_send ? "can" : "cannot", db_owner_info->dbID);
		} else {
			fprintf(stderr,
			    "isds_PDZSendInfo() failed: %s: %s\n",
			    isds_strerror(err), isds_long_message(ctx));
			ret = EXIT_FAILURE;
			goto fail;
		}
	}

	ret = EXIT_SUCCESS;

fail:
	isds_list_free(&permissions);
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

	return ret;
}
