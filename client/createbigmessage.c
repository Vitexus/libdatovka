#define _XOPEN_SOURCE 500 /* strdup() */
#include <libdatovka/isds.h>
#include <locale.h> /* setlocale() */
#include <stdbool.h>
#include <stdlib.h>
#include <string.h> /* memset(), strdup() */

#include "common.h"

#define BIG_DATA_MAX (24 * 1024 * 1024)
//#define BIG_DATA_MAX (24 * 102)

int main(void)
{
	struct isds_ctx *ctx = NULL;
	enum isds_error err = IE_SUCCESS;
	int ret = EXIT_FAILURE;
	char *recipient = NULL;
	char *big_data = NULL;
	size_t big_data_size = 0;
	struct isds_dmAtt *dm_att = NULL;

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

	err = isds_ping(ctx);
	if (err != IE_SUCCESS) {
		printf("isds_ping() failed: %s\n", isds_strerror(err));
	} else {
		fputs("isds_ping() succeeded\n", stdout);
	}

	fflush(stdout);

	{
		struct isds_list *boxes = NULL, *item;
		struct isds_DbOwnerInfo criteria;
		isds_DbType criteria_db_type = DBTYPE_PO;
		memset(&criteria, 0, sizeof(criteria));
		criteria.firmName = "Barbucha";
		criteria.dbType = &criteria_db_type;

		printf("Searching box with firm name `%s':\n", criteria.firmName);
		err = isds_FindDataBox(ctx, &criteria, &boxes);
		if ((err == IE_SUCCESS) || (err == IE_TOO_BIG)) {
			fputs("isds_FindDataBox() succeeded:\n", stdout);

			int n;
			for(item = boxes, n = 1; item; item = item->next, n++) {
				if (err != IE_TOO_BIG) {
					printf("List item #%d:\n", n);
					print_DbOwnerInfo(item->data);
				}
				if (n == 1) {
					free(recipient);
					recipient = strdup(
					    ((struct isds_DbOwnerInfo *)(item->data))->dbID);
				}
			}
			if (err == IE_TOO_BIG) {
				printf("isds_FindDataBox() results truncated to %d boxes\n",
				    --n);
			}
		} else {
			printf("isds_FindDataBox() failed: %s: %s\n",
			    isds_strerror(err), isds_long_message(ctx));
		}

		isds_list_free(&boxes);
	}

	fflush(stdout);

	/* Create big attachment. */
	{
		const char *line = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
		const size_t line_len = strlen(line);
		struct isds_dmFile dm_file = {0, };
		const char *file_name = "test.txt";
		const char *suff = strchr(file_name, '.');
		if (NULL != suff) {
			++suff;
		}

		big_data = malloc(BIG_DATA_MAX);
		if (NULL == big_data) {
			fputs("Not enough memory.\n", stderr);
			goto fail;
		}
		while ((big_data_size + line_len) <= BIG_DATA_MAX) {
			memcpy(big_data + big_data_size, line, line_len);
			big_data_size += line_len;
		}

		dm_file.data = big_data;
		dm_file.data_length = big_data_size;

		dm_file.dmMimeType = (char *)isds_normalize_mime_type(suff);
		if (NULL == dm_file.dmMimeType) {
			fprintf(stderr,
			    "Cannot determine MIME type of file '%s'.\n",
			    file_name);
			ret = EXIT_FAILURE;
			goto leave;
		}
		printf("Detected MIME type '%s' for file '%s'.\n",
		    dm_file.dmMimeType, file_name);

		dm_file.dmFileDescr = (char *)file_name;

		err = isds_UploadAttachment_mtomxop(ctx, &dm_file, &dm_att);
		if (err != IE_SUCCESS) {
			fprintf(stderr, "isds_UploadAttachment_mtomxop() failed: %s: %s\n",
			    isds_strerror(err), isds_long_message(ctx));
			ret = EXIT_FAILURE;
			goto leave;
		} else {
			printf("isds_UploadAttachment_mtomxop() succeeded\n");
		}

		free(big_data);

		if (0) {
leave:
			free(big_data);
			goto fail;
		}
	}

	fflush(stdout);

	/* Send big message. */
	{
		struct isds_message message;
		memset(&message, 0, sizeof(message));

		struct isds_envelope envelope;
		memset(&envelope, 0, sizeof(envelope));
		message.envelope = &envelope;
		envelope.dbIDRecipient = recipient;
		long int dmSenderOrgUnitNum = 42;
		envelope.dmSenderOrgUnitNum = &dmSenderOrgUnitNum;
		envelope.dmAnnotation = "Big Message Test";

		_Bool dmPublishOwnID = true;
		envelope.dmPublishOwnID = &dmPublishOwnID;
		int idLevel = PUBLISH_USERTYPE | PUBLISH_PERSONNAME | PUBLISH_BIDATE
		    | PUBLISH_BICITY | PUBLISH_BICOUNTY | PUBLISH_ADCODE
		    | PUBLISH_FULLADDRESS | PUBLISH_ROBIDENT;
		envelope.idLevel = &idLevel;

		struct isds_document minor_document;
		memset(&minor_document, 0, sizeof(minor_document));
		minor_document.data = "hello world?";
		minor_document.data_length = strlen(minor_document.data);
		minor_document.dmMimeType = "text/plain";
		/* XXX: This should fail */
		/* minor_document.dmFileMetaType = FILEMETATYPE_MAIN; */
		minor_document.dmFileMetaType = FILEMETATYPE_ENCLOSURE;
		/* Server implementation demands dmFileDescr to be valid file name */
		/*minor_document.dmFileDescr = "Standard text.txt";*/
		minor_document.dmFileDescr = "minor_standard_text.txt";
		minor_document.dmFileGuid = "2";
		minor_document.dmUpFileGuid = "1";

		struct isds_document main_document;
		memset(&main_document, 0, sizeof(main_document));
		main_document.data = "Hello World!";
		main_document.data_length = strlen(main_document.data);
		/* Server implementation says text is not text file
		 * See <http://www.abclinuxu.cz/forum/show/284940> */
		main_document.dmMimeType = "text/plain";
		/* XXX: This should fail */
		/* main_document.dmFileMetaType = FILEMETATYPE_MAIN; */
		main_document.dmFileMetaType = FILEMETATYPE_ENCLOSURE;
		/* Server implementation demands dmFileDescr to be valid file name */
		/*main_document.dmFileDescr = "Standard text.txt";*/
		main_document.dmFileDescr = "standard_text.txt";
		main_document.dmFileGuid = "1";

		struct isds_list documents_main_item = {
			.data = &main_document,
			.next = NULL,
			.destructor = NULL
		};
		struct isds_list documents_minor_item = {
			.data = &minor_document,
			.next = &documents_main_item,
			.destructor = NULL
		};
		message.documents = &documents_minor_item;

		struct isds_dmExtFile ext_file = {
			/* XXX: This should fail */
			/* .dmFileMetaType = FILEMETATYPE_ENCLOSURE, */
			.dmFileMetaType = FILEMETATYPE_MAIN,
			.dmAtt = *dm_att
		};

		struct isds_list ext_file_main_item = {
			.data = &ext_file,
			.next = NULL,
			.destructor = NULL
		};
		message.ext_files = &ext_file_main_item;

		printf("Sending big message to box ID `%s'\n",
		    message.envelope->dbIDRecipient);
		err = isds_CreateBigMessage(ctx, &message);
		if (err != IE_SUCCESS) {
			printf("isds_CreateBigMessage() failed: %s: %s\n",
			    isds_strerror(err), isds_long_message(ctx));
		} else {
			printf("isds_CreateBigMessage() succeeded: assigned message ID = %s\n",
			    message.envelope->dmID);
		}

		free(message.envelope->dmID);
	}

	fflush(stdout);

	free(recipient); recipient = NULL;
	isds_dmAtt_free(&dm_att);

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
