#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <string.h>
#include <libdatovka/isds.h>
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

/* Get info about user */
static isds_error get_user(struct isds_ctx *ctx,
    struct isds_DbUserInfoExt2 **db_user_info)
{
	isds_error err = IE_SUCCESS;
	printf("Getting info about my account:\n");
	err = isds_GetUserInfoFromLogin2(ctx, db_user_info);
	if (err != IE_SUCCESS) {
		fprintf(stderr, "isds_GetUserInfoFromLogin2() failed: %s: %s\n",
		    isds_strerror(err), isds_long_message(ctx));
	} else {
		printf("isds_GetUserInfoFromLogin2() succeeded\n");
		print_DbUserInfoExt2(*db_user_info);
	}
	return err;
}

static char *join_str(const char *str1, const char *str2)
{
	char *str = NULL;
	size_t len1 = 0;
	size_t len2 = 0;

	if (str1 != NULL) {
		len1 = strlen(str1);
	}
	if (str2 != NULL) {
		len2 = strlen(str2);
	}
	if ((str1 != NULL) || (str2 != NULL)) {
		str = calloc(1, len1 + len2 + 1);
		if (str == NULL) {
			fprintf(stderr, "Not enough memory\n");
			exit(EXIT_FAILURE);
		}
	}
	if (str1 != NULL) {
		memcpy(str, str1, len1);
	}
	if (str2 != NULL) {
		memcpy(str + len1, str2, len2);
	}
	if (str != NULL) {
		str[len1 + len2] = '\0';
	}
	return str;
}

static _Bool str_eq(const char *s1, const char *s2)
{
	return (s1 == s2) || ((s1 != NULL) && (s2 != NULL) && !strcmp(s1, s2));
}

static _Bool isds_PersonName2_eq(const struct isds_PersonName2 *pn1,
    const struct isds_PersonName2 *pn2)
{
	return (pn1 == pn2) ||
	    (str_eq(pn1->pnGivenNames, pn2->pnGivenNames) &&
	        str_eq(pn1->pnLastName, pn2->pnLastName));
}

static _Bool isds_AddressExt2_eq(const struct isds_AddressExt2 *a1,
    const struct isds_AddressExt2 *a2)
{
	return (a1 == a2) ||
	    (str_eq(a1->adCode, a2->adCode) &&
	        str_eq(a1->adCity, a2->adCity) &&
	        str_eq(a1->adDistrict, a2->adDistrict) &&
	        str_eq(a1->adStreet, a2->adStreet) &&
	        str_eq(a1->adNumberInStreet, a2->adNumberInStreet) &&
	        str_eq(a1->adNumberInMunicipality, a2->adNumberInMunicipality) &&
	        str_eq(a1->adZipCode, a2->adZipCode) &&
	        str_eq(a1->adState, a2->adState));
}

static _Bool isds_DbUserInfoExt2_match(const struct isds_DbUserInfoExt2 *u1,
    const struct isds_DbUserInfoExt2 *u2)
{
	return (u1 == u2) ||
	    (isds_PersonName2_eq(u1->personName, u2->personName) &&
	        isds_AddressExt2_eq(u1->address, u2->address));
}

int main(void)
{
	struct isds_ctx *ctx = NULL;
	isds_error err;
	struct isds_DbOwnerInfoExt2 *db_owner_info = NULL;
	struct isds_DbUserInfoExt2 *user_existing = NULL;
	struct isds_DbUserInfoExt2 *user_to_be_created = NULL;
	struct isds_DbUserInfoExt2 *user_actually_created = NULL;
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

	err = get_user(ctx, &user_existing);
	if (err != IE_SUCCESS) {
		ret = EXIT_FAILURE;
		goto fail;
	}

	/* Alter existing user description. */
	{
		user_to_be_created = isds_DbUserInfoExt2_duplicate(user_existing);
		if (user_to_be_created == NULL) {
			fprintf(stderr, "Not enough memory\n");
			ret = EXIT_FAILURE;
			goto fail;
		}

		/* Modify name. */
		char *aux = user_to_be_created->personName->pnGivenNames;
		user_to_be_created->personName->pnGivenNames = join_str("Another ", aux);
		free(aux); aux  = NULL;

		if (user_to_be_created->address == NULL) {
			user_to_be_created->address =
			    calloc(1, sizeof(*user_to_be_created->address));
			if (user_to_be_created->address == NULL) {
				fprintf(stderr, "Not enough memory\n");
				ret = EXIT_FAILURE;
				goto fail;
			}
		}
		/* Add address if missing. */
		{
			struct isds_AddressExt2 *address =
			    user_to_be_created->address;

			if (address->adCity == NULL) {
				address->adCity =
				    join_str("Oberwerwolfsbruck", NULL);
			}
			if (address->adStreet == NULL) {
				address->adStreet =
				    join_str("Pappenheimergasse", NULL);
			}
			if (address->adNumberInStreet == NULL) {
				address->adNumberInStreet = join_str("6", NULL);
			}
			if (address->adNumberInMunicipality == NULL) {
				address->adNumberInMunicipality =
				    join_str("1224", NULL);
			}
			if (address->adZipCode == NULL) {
				address->adZipCode = join_str("86625", NULL);
			}
		}

		/* Newly created user does not have its internal id obviously. */
		free(user_to_be_created->isdsID); user_to_be_created->isdsID = NULL;
		/* According to the documentation only entrusted users can be added. */
		*user_to_be_created->userType = USERTYPE_ENTRUSTED;
		/* Give low privileges. */
		*user_to_be_created->userPrivils = PRIVIL_VIEW_INFO | PRIVIL_SEARCH_DB;
	}

	/* Create new user using altered data. */
	if (user_to_be_created != NULL) {
		char *refnumber = NULL;

		printf("Attempting to add user to data box with ID `%s':\n",
		    db_owner_info->dbID);
		print_DbUserInfoExt2(user_to_be_created);

		err = isds_AddDataBoxUser2(ctx, db_owner_info->dbID, user_to_be_created,
		    NULL, NULL, &refnumber);
		if (err != IE_SUCCESS) {
			fprintf(stderr, "isds_AddDataBoxUser2() failed: %s: %s\n",
			    isds_strerror(err), isds_long_message(ctx));
			ret = EXIT_FAILURE;
			goto fail;
		} else {
			printf(
			    "isds_AddDataBoxUser2() succeeded with reference number: %s\n",
			    refnumber);
		}
		const struct isds_status *status = isds_operation_status(ctx);
		if (status != NULL) {
			printf(
			    "Obtained status code: '%s'; message: '%s'; reference number: '%s'\n",
			    status->code, status->message, status->ref_number);
		} else {
			fprintf(stderr, "Cannot obtain status after calling isds_AddDataBoxUser2()\n");
			ret = EXIT_FAILURE;
			goto fail;
		}

		free(refnumber); refnumber = NULL;
	}

	/*
	 * When creating a new user in the public test environment the user
	 * credentials are returned inside the status:
	 * <p:dbStatus>
	 *     <p:dbStatusCode>0000</p:dbStatusCode>
	 *     <p:dbStatusMessage>Provedeno úspěšně.Login mh6mg9 password Sq8-D_FQHq</p:dbStatusMessage>
	 *     <p:dbStatusRefNumber>REF187124</p:dbStatusRefNumber>
	 * </p:dbStatus>
	 *
	 * The only way how to obtain this information is from the log or
	 * by acquiring the status via isds_operation_status().
	 */

	/* Download user data and search for newly created user. */
	{
		struct isds_list *users = NULL, *item;

		printf("Getting users box with ID `%s':\n", db_owner_info->dbID);
		err = isds_GetDataBoxUsers2(ctx, db_owner_info->dbID, &users);
		if (err != IE_SUCCESS) {
			fprintf(stderr,
			    "isds_GetDataBoxUsers2() failed: %s: %s\n",
			    isds_strerror(err), isds_long_message(ctx));
		} else {
			printf("isds_GetDataBoxUsers2() succeeded\n");
			for(item = users; item; item = item->next) {
				const struct isds_DbUserInfoExt2 *user = item->data;
				if (isds_DbUserInfoExt2_match(user_to_be_created, user)) {
					if (user_actually_created == NULL) {
						user_actually_created =
						    isds_DbUserInfoExt2_duplicate(user);
						if (user_actually_created == NULL) {
							fprintf(stderr,
							    "Not enough memory\n");
							ret = EXIT_FAILURE;
							goto fail;
						}
					} else {
						fprintf(stderr,
						    "Found more than one matching user entry\n");
						ret = EXIT_FAILURE;
						goto fail;
					}
				}
			}
		}

		isds_list_free(&users);
	}

	/* Modify user name and privileges. */
	if (user_actually_created) {
		char *refnumber = NULL;
		char *aux = user_actually_created->personName->pnGivenNames;

		user_actually_created->personName->pnGivenNames = join_str("Yet ", aux);
		if (user_actually_created->personName->pnGivenNames == NULL) {
			fprintf(stderr, "Not enough memory\n");
			ret = EXIT_FAILURE;
			goto fail;
		}

		*user_to_be_created->userPrivils = PRIVIL_VIEW_INFO;

		free(aux); aux = NULL;

		printf("Updating newly added user from data box with ID `%s':\n",
		    db_owner_info->dbID);

		err = isds_UpdateDataBoxUser2(ctx, db_owner_info->dbID,
		    user_actually_created->isdsID, user_actually_created,
		    &refnumber);
		if (err != IE_SUCCESS) {
			fprintf(stderr,
			    "isds_UpdateDataBoxUser2() failed: %s: %s\n",
			    isds_strerror(err), isds_long_message(ctx));
			ret = EXIT_FAILURE;
			goto fail;
		} else {
			printf(
			    "isds_UpdateDataBoxUser2() succeeded with reference number: %s\n",
			    refnumber);
		}

		free(refnumber); refnumber = NULL;
	}

	/* Delete the newly added user. */
	if (user_actually_created != NULL) {
		char *refnumber = NULL;

		printf("Deleting newly added user from data box with ID `%s':\n",
		    db_owner_info->dbID);
		print_DbUserInfoExt2(user_actually_created);

		err = isds_DeleteDataBoxUser2(ctx, db_owner_info->dbID,
		    user_actually_created->isdsID, NULL, &refnumber);
		if (err != IE_SUCCESS) {
			fprintf(stderr, "isds_DeleteDataBoxUser2() failed: %s: %s\n",
			    isds_strerror(err), isds_long_message(ctx));
			ret = EXIT_FAILURE;
			goto fail;
		} else {
			printf(
			    "isds_DeleteDataBoxUser2() succeeded with reference number: %s\n",
			    refnumber);
		}

		free(refnumber); refnumber = NULL;
	}

	ret = EXIT_SUCCESS;

fail:
	isds_DbUserInfoExt2_free(&user_actually_created);
	isds_DbUserInfoExt2_free(&user_to_be_created);
	isds_DbUserInfoExt2_free(&user_existing);
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
