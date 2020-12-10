#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <string.h>
#include <libdatovka/isds.h>
#include "common.h"

/* Get info about owner. */
static isds_error get_owner(struct isds_ctx *ctx,
    struct isds_DbOwnerInfo **db_owner_info)
{
	isds_error err = IE_SUCCESS;
	printf("Getting info about my box:\n");
	err = isds_GetOwnerInfoFromLogin(ctx, db_owner_info);
	if (err != IE_SUCCESS) {
		fprintf(stderr, "isds_GetOwnerInfoFromLogin() failed: %s: %s\n",
		    isds_strerror(err), isds_long_message(ctx));
	} else {
		printf("isds_GetOwnerInfoFromLogin() succeeded\n");
		print_DbOwnerInfo(*db_owner_info);
	}
	return err;
}

/* Get info about user */
static isds_error get_user(struct isds_ctx *ctx,
    struct isds_DbUserInfo **db_user_info)
{
	isds_error err = IE_SUCCESS;
	printf("Getting info about my account:\n");
	err = isds_GetUserInfoFromLogin(ctx, db_user_info);
	if (err != IE_SUCCESS) {
		fprintf(stderr, "isds_GetUserInfoFromLogin() failed: %s: %s\n",
		    isds_strerror(err), isds_long_message(ctx));
	} else {
		printf("isds_GetUserInfoFromLogin() succeeded\n");
		print_DbUserInfo(*db_user_info);
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

static _Bool isds_PersonName_eq(const struct isds_PersonName *pn1,
    const struct isds_PersonName *pn2)
{
	return (pn1 == pn2) ||
	    (str_eq(pn1->pnFirstName, pn2->pnFirstName) &&
	        str_eq(pn1->pnMiddleName, pn2->pnMiddleName) &&
	        str_eq(pn1->pnLastName, pn2->pnLastName) &&
	        str_eq(pn1->pnLastNameAtBirth, pn2->pnLastNameAtBirth));
}

static _Bool isds_Address_eq(const struct isds_Address *a1,
    const struct isds_Address *a2)
{
	return (a1 == a2) ||
	    (str_eq(a1->adCity, a2->adCity) &&
	        str_eq(a1->adStreet, a2->adStreet) &&
	        str_eq(a1->adNumberInStreet, a2->adNumberInStreet) &&
	        str_eq(a1->adNumberInMunicipality, a2->adNumberInMunicipality) &&
	        str_eq(a1->adZipCode, a2->adZipCode) &&
	        str_eq(a1->adState, a2->adState));
}

static _Bool isds_DbUserInfo_match(const struct isds_DbUserInfo *u1,
    const struct isds_DbUserInfo *u2)
{
	return (u1 == u2) ||
	    (isds_PersonName_eq(u1->personName, u2->personName) &&
	        isds_Address_eq(u1->address, u2->address));
}

int main(void)
{
	struct isds_ctx *ctx = NULL;
	isds_error err;
	struct isds_DbOwnerInfo *db_owner_info = NULL;
	struct isds_DbUserInfo *user_existing = NULL;
	struct isds_DbUserInfo *user_to_be_created = NULL;
	struct isds_DbUserInfo *user_actually_created = NULL;

	setlocale(LC_ALL, "");

	err = isds_init();
	if (err != IE_SUCCESS) {
		fprintf(stderr, "isds_init() failed: %s\n", isds_strerror(err));
		exit(EXIT_FAILURE);
	}

	isds_set_logging(ILF_ALL & ~ILF_HTTP, ILL_ALL);

	ctx = isds_ctx_create();
	if (ctx == NULL) {
		fprintf(stderr, "isds_ctx_create() failed\n");
		exit(EXIT_FAILURE);
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
		exit(EXIT_FAILURE);
	} else {
		printf("Logged in :)\n");
	}

	err = get_owner(ctx, &db_owner_info);
	if (err != IE_SUCCESS) {
		exit(EXIT_FAILURE);
	}

	err = get_user(ctx, &user_existing);
	if (err != IE_SUCCESS) {
		exit(EXIT_FAILURE);
	}

	/* Alter existing user description. */
	{
		user_to_be_created = isds_DbUserInfo_duplicate(user_existing);
		if (user_to_be_created == NULL) {
			fprintf(stderr, "Not enough memory\n");
			exit(EXIT_FAILURE);
		}

		/* Modify name. Add first to middle and replace first. */
		char *aux = NULL;
		if (user_to_be_created->personName->pnMiddleName != NULL) {
			aux = user_to_be_created->personName->pnMiddleName;
			user_to_be_created->personName->pnMiddleName =
			    join_str(" ", aux);
			free(aux); aux  = NULL;
		}
		if (user_to_be_created->personName->pnFirstName != NULL) {
			aux = user_to_be_created->personName->pnMiddleName;
			user_to_be_created->personName->pnMiddleName =
			    join_str(user_to_be_created->personName->pnFirstName, aux);
			free(aux); aux  = NULL;
		}
		free(user_to_be_created->personName->pnFirstName);
		user_to_be_created->personName->pnFirstName =
		    join_str("Another", NULL);

		if (user_to_be_created->address != NULL) {
			user_to_be_created->address =
			    calloc(1, sizeof(*user_to_be_created->address));
			if (user_to_be_created->address == NULL) {
				fprintf(stderr, "Not enough memory\n");
				exit(EXIT_FAILURE);
			}
		}
		/* Add address if missing. */
		{
			struct isds_Address *address =
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
				address->adNumberInStreet =
				    join_str("6", NULL);
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
		free(user_to_be_created->userID); user_to_be_created->userID = NULL;
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
		print_DbUserInfo(user_to_be_created);

		err = isds_add_user(ctx, db_owner_info, user_to_be_created,
		    NULL, NULL, &refnumber);
		if (err != IE_SUCCESS) {
			fprintf(stderr, "isds_add_user() failed: %s: %s\n",
			    isds_strerror(err), isds_long_message(ctx));
			exit(EXIT_FAILURE);
		} else {
			printf(
			    "isds_add_user() succeeded with reference number: %s\n",
			    refnumber);
		}
		const struct isds_status *status = isds_operation_status(ctx);
		if (status != NULL) {
			printf(
			    "Obtained status code: '%s'; message: '%s'; reference number: '%s'\n",
			    status->code, status->message, status->ref_number);
		} else {
			fprintf(stderr, "Cannot obtain status after calling isds_add_user()\n");
			exit(EXIT_FAILURE);
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
		err = isds_GetDataBoxUsers(ctx, db_owner_info->dbID, &users);
		if (err != IE_SUCCESS) {
			fprintf(stderr,
			    "isds_GetDataBoxUsers() failed: %s: %s\n",
			    isds_strerror(err), isds_long_message(ctx));
		} else {
			printf("isds_GetDataBoxUsers() succeeded\n");
			for(item = users; item; item = item->next) {
				const struct isds_DbUserInfo *user = item->data;
				if (isds_DbUserInfo_match(user_to_be_created, user)) {
					if (user_actually_created == NULL) {
						user_actually_created =
						    isds_DbUserInfo_duplicate(user);
						if (user_actually_created == NULL) {
							fprintf(stderr,
							    "Not enough memory\n");
							exit(EXIT_FAILURE);
						}
					} else {
						fprintf(stderr,
						    "Found more than one matching user entry\n");
						exit(EXIT_FAILURE);
					}
				}
			}
		}

		isds_list_free(&users);
	}

	/* Modify user name and privileges. */
	if (user_actually_created != NULL) {
		char *refnumber = NULL;
		struct isds_DbUserInfo *aux = NULL;

		aux = isds_DbUserInfo_duplicate(user_actually_created);
		if (aux == NULL) {
			fprintf(stderr, "Not enough memory\n");
			exit(EXIT_FAILURE);
		}

		user_actually_created->personName->pnFirstName =
		    join_str("Yet ", aux->personName->pnFirstName);
		if (user_actually_created->personName->pnFirstName == NULL) {
			fprintf(stderr, "Not enough memory\n");
			exit(EXIT_FAILURE);
		}

		*user_to_be_created->userPrivils = PRIVIL_VIEW_INFO;

		printf("Updating newly added user from data box with ID `%s':\n",
		    db_owner_info->dbID);

		err = isds_UpdateDataBoxUser(ctx, db_owner_info, aux,
		    user_actually_created, &refnumber);
		if (err != IE_SUCCESS) {
			fprintf(stderr,
			    "isds_UpdateDataBoxUser() failed: %s: %s\n",
			    isds_strerror(err), isds_long_message(ctx));
			exit(EXIT_FAILURE);
		} else {
			printf(
			    "isds_UpdateDataBoxUser() succeeded with reference number: %s\n",
			    refnumber);
		}

		isds_DbUserInfo_free(&aux);
		free(refnumber); refnumber = NULL;
	}

	/* Delete the newly added user. */
	if (user_actually_created != NULL) {
		char *refnumber = NULL;

		printf("Deleting newly added user from data box with ID `%s':\n",
		        db_owner_info->dbID);
		print_DbUserInfo(user_actually_created);

		err = isds_delete_user(ctx, db_owner_info,
		    user_actually_created, NULL, &refnumber);
		if (err != IE_SUCCESS) {
			fprintf(stderr, "isds_delete_user() failed: %s: %s\n",
			    isds_strerror(err), isds_long_message(ctx));
			exit(EXIT_FAILURE);
		} else {
			printf(
			    "isds_delete_user() succeeded with reference number: %s\n",
			    refnumber);
		}

		free(refnumber); refnumber = NULL;
	}

	isds_DbUserInfo_free(&user_actually_created);
	isds_DbUserInfo_free(&user_to_be_created);
	isds_DbUserInfo_free(&user_existing);
	isds_DbOwnerInfo_free(&db_owner_info);

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

	exit (EXIT_SUCCESS);
}
