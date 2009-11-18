#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <time.h>
#include <string.h>
#include <isds.h>

char url[] = "https://www.czebox.cz/DS/";
char username[] = "jrfh7i";
char password[] = "Ab123456";
struct isds_ctx *ctx;


void print_DbState(const long int state) {
    switch(state) {
        case DBSTATE_ACCESSIBLE: printf("ACCESSIBLE\n"); break;
        case DBSTATE_TEMP_UNACCESSIBLE: printf("TEMP_UNACCESSIBLE\n"); break;
        case DBSTATE_NOT_YET_ACCESSIBLE: printf("NOT_YET_ACCESSIBLE\n"); break;
        case DBSTATE_PERM_UNACCESSIBLE: printf("PERM_UNACCESSIBLE\n"); break;
        case DBSTATE_REMOVED: printf("REMOVED\n"); break;
        default: printf("<unknown state %ld>\n", state);
    }
}


void print_DbOwnerInfo(struct isds_DbOwnerInfo *info) {
    printf("dbOwnerInfo = ");

    if (!info) {
        printf("NULL\n");
        return;
    }

    printf("{\n");
    printf("\tdbID = %s\n", info->dbID);

    printf("\tdbType = ");
    if (!info->dbType) printf("NULL\n");
    else
        switch(*(info->dbType)) {
            case DBTYPE_FO: printf("FO\n"); break;
            case DBTYPE_PFO: printf("PFO\n"); break;
            case DBTYPE_PFO_ADVOK: printf("PFO_ADVOK\n"); break;
            case DBTYPE_PFO_DANPOR: printf("PFO_DAPOR\n"); break;
            case DBTYPE_PFO_INSSPR: printf("PFO_INSSPR\n"); break;
            case DBTYPE_PO: printf("PO\n"); break;
            case DBTYPE_PO_ZAK: printf("PO_ZAK\n"); break;
            case DBTYPE_PO_REQ: printf("PO_REQ\n"); break;
            case DBTYPE_OVM: printf("OVM\n"); break;
            case DBTYPE_OVM_NOTAR: printf("OVM_NOTAR\n"); break;
            case DBTYPE_OVM_EXEKUT: printf("OVM_EXEKUT\n"); break;
            case DBTYPE_OVM_REQ: printf("OVM_REQ\n"); break;
            default: printf("<unknown type %d>\n", *(info->dbType));
        }
    printf("\tic = %s\n", info->ic);

    printf("\tpersonName = ");
    if (!info->personName) printf("NULL\n");
    else {
        printf("{\n");
        printf("\t\tpnFirstName = %s\n", info->personName->pnFirstName);
        printf("\t\tpnMiddleName = %s\n", info->personName->pnMiddleName);
        printf("\t\tpnLastName = %s\n", info->personName->pnLastName);
        printf("\t\tpnLastNameAtBirth = %s\n", info->personName->pnLastNameAtBirth);
        printf("\t}\n");
    }
        
    printf("\tfirmName = %s\n", info->firmName);
    
    printf("\tbirthInfo = ");
    if (!info->birthInfo) printf("NULL\n");
    else {
        printf("{\n");
        
        printf("\t\tbiDate = ");
        if (!info->birthInfo->biDate) printf("NULL\n");
        else printf("%s\n", asctime(info->birthInfo->biDate));

        printf("\t\tbiCity = %s\n", info->birthInfo->biCity);
        printf("\t\tbiCounty = %s\n", info->birthInfo->biCounty);
        printf("\t\tbiState = %s\n", info->birthInfo->biState);
        printf("\t}\n");
    }
    
    printf("\taddress = ");
    if (!info->address) printf("NULL\n");
    else {
        printf("{\n");
        printf("\t\tadCity = %s\n", info->address->adCity);
        printf("\t\tadStreet = %s\n", info->address->adStreet);
        printf("\t\tadNumberInStreet = %s\n", info->address->adNumberInStreet);
        printf("\t\tadNumberInMunicipality = %s\n",
                info->address->adNumberInMunicipality);
        printf("\t\tadZipCode = %s\n", info->address->adZipCode);
        printf("\t\tadState = %s\n", info->address->adState);
        printf("\t}\n");
    }

    printf("\tnationality = %s\n", info->nationality);
    printf("\temail = %s\n", info->email);
    printf("\ttelNumber = %s\n", info->telNumber);
    printf("\tidentifier = %s\n", info->identifier);
    printf("\tregistryCode = %s\n", info->registryCode);

    printf("\tdbState = ");
    if (!info->dbState) printf("NULL\n");
    else print_DbState(*(info->dbState));
    
    printf("\tdbEffectiveOVM = %s\n",
            !info->dbEffectiveOVM ? "NULL" :
                (*(info->dbEffectiveOVM)? "true" : "false"));

    printf("\tdbOpenAddressing = %s\n",
            !info->dbOpenAddressing ? "NULL" :
                (*(info->dbOpenAddressing)? "true" : "false"));

    printf("}\n");

}


int main(int argc, char **argv) {
    isds_error err;
    struct isds_DbOwnerInfo *db_owner_info = NULL;
    char *recipient = NULL;
    
    setlocale(LC_ALL, "");

    err = isds_init();
    if (err) {
        printf("isds_init() failed: %s\n", isds_strerror(err));
    }

    isds_set_logging(ILF_ALL & ~ILF_HTTP, ILL_ALL);

    ctx = isds_ctx_create();
    if (!ctx) {
        printf("isds_ctx_create() failed");
    }

    err = isds_set_timeout(ctx, 10000);
    if (err) {
        printf("isds_set_timeout() failed: %s\n", isds_strerror(err));
    }

    err = isds_login(ctx, url, username, password, NULL, NULL);
    if (err) {
        printf("isds_login() failed: %s: %s\n", isds_strerror(err),
                isds_long_message(ctx));
    } else {
        printf("Logged in :)\n");
    }

    err = isds_ping(ctx);
    if (err) {
        printf("isds_ping() failed: %s\n", isds_strerror(err));
    } else {
        printf("isds_ping() succeeded\n");
    }

    printf("Sending bogus request\n");
    err = isds_bogus_request(ctx);
    if (err) {
        printf("isds_bogus_request() failed: %s: %s\n", isds_strerror(err),
                isds_long_message(ctx));
    } else {
        printf("isds_bogus_request() succeeded\n");
    }

    {
        printf("Getting info about my box:\n");
        err = isds_GetOwnerInfoFromLogin(ctx, &db_owner_info);
        if (err) {
            printf("isds_GetOwnerInfoFromLogin() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        } else {
            printf("isds_GetOwnerInfoFromLogin() succeeded\n");
        }
        print_DbOwnerInfo(db_owner_info);

    }

    {
        /* Current server implementation (2009-11-17) does not allow to find
         * myself. Previous version allowed it. */
        struct isds_list *boxes = NULL, *item;

        printf("Searching for my own box:\n");
        err = isds_FindDataBox(ctx, db_owner_info, &boxes);
        if (err == IE_SUCCESS || err == IE_2BIG) {
            if (err == IE_2BIG) 
                printf("isds_FindDataBox() results truncated\n");
            printf("isds_FindDataBox() succeeded:\n");

            for(item = boxes; item; item = item->next) {
                printf("List item:\n");
                print_DbOwnerInfo(item->data);
            }
        } else {
            printf("isds_FindDataBox() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        }

        isds_list_free(&boxes);
    }

    {
        struct isds_list *boxes = NULL, *item;
        struct isds_DbOwnerInfo *criteria = calloc(1, sizeof(*criteria));
        if (!criteria) {
            printf("Not enough memory for struct isds_DbOwnerInfo criteria\n");
            exit(-1);
        }
        criteria->firmName = strdup("Obec");
        if (!criteria->firmName) {
            printf("Not enough memory for criteria->firmName\n");
            exit(-1);
        }
        criteria->dbType = malloc(sizeof(*(criteria->dbType)));
        if (!criteria->dbType) {
            printf("Not enough memory for criteria->dbType\n");
            exit(-1);
        }
        *(criteria->dbType) = DBTYPE_OVM;

        printf("Searching box with firm name `%s':\n", criteria->firmName);
        err = isds_FindDataBox(ctx, criteria, &boxes);
        if (err == IE_SUCCESS || err == IE_2BIG) {
            printf("isds_FindDataBox() succeeded:\n");

            int n;
            for(item = boxes, n = 1; item; item = item->next, n++) {
                if (err != IE_2BIG) {
                    printf("List item #%d:\n", n);
                    print_DbOwnerInfo(item->data);
                }
            }
            if (err == IE_2BIG) 
                printf("isds_FindDataBox() results truncated to %d boxes\n",
                        --n);
        } else {
            printf("isds_FindDataBox() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        }

        isds_list_free(&boxes);
        isds_DbOwnerInfo_free(&criteria);
    }

    {
        struct isds_list *boxes = NULL, *item;
        struct isds_DbOwnerInfo criteria;
        isds_DbType criteria_db_type = DBTYPE_OVM;
        memset(&criteria, 0, sizeof(criteria));
        criteria.dbType = &criteria_db_type;
        criteria.dbID = "vqbab52";

        printf("Searching for exact box by ID `%s' and type:\n", criteria.dbID);
        err = isds_FindDataBox(ctx, &criteria, &boxes);
        if (err == IE_SUCCESS || err == IE_2BIG) {
            printf("isds_FindDataBox() succeeded:\n");

            int n;
            for(item = boxes, n = 1; item; item = item->next, n++) {
                if (err != IE_2BIG) {
                    printf("List item #%d:\n", n);
                    print_DbOwnerInfo(item->data);
                }
                if (n == 1) recipient = strdup(
                        ((struct isds_DbOwnerInfo *)(item->data))->dbID);
            }
            if (err == IE_2BIG) 
                printf("isds_FindDataBox() results truncated to %d boxes\n",
                        --n);
        } else {
            printf("isds_FindDataBox() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        }

        isds_list_free(&boxes);
    }


    {
        struct isds_list *boxes = NULL, *item;
        struct isds_DbOwnerInfo criteria;
        isds_DbType criteria_db_type = DBTYPE_OVM;
        memset(&criteria, 0, sizeof(criteria));
        criteria.firmName = "Místní";
        criteria.dbType = &criteria_db_type;

        printf("Searching box with firm name `%s':\n", criteria.firmName);
        err = isds_FindDataBox(ctx, &criteria, &boxes);
        if (err == IE_SUCCESS || err == IE_2BIG) {
            printf("isds_FindDataBox() succeeded:\n");

            int n;
            for(item = boxes, n = 1; item; item = item->next, n++) {
                if (err != IE_2BIG) {
                    printf("List item #%d:\n", n);
                    print_DbOwnerInfo(item->data);
                }
                if (n == 1) recipient = strdup(
                        ((struct isds_DbOwnerInfo *)(item->data))->dbID);
            }
            if (err == IE_2BIG) 
                printf("isds_FindDataBox() results truncated to %d boxes\n",
                        --n);
        } else {
            printf("isds_FindDataBox() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        }

        isds_list_free(&boxes);
    }


    if (db_owner_info) {
        long int box_status = 0;
        printf("Getting status of my box with ID `%s'\n", db_owner_info->dbID);
        err = isds_CheckDataBox(ctx, db_owner_info->dbID, &box_status);
        if (err)
            printf("isds_CheckDataBox() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        else {
            printf("isds_CheckDataBox() succeeded: status = ");
            print_DbState(box_status);
        }
    }

    isds_DbOwnerInfo_free(&db_owner_info);

    {
        char *box_id = "7777777";
        long int box_status = 0;
        printf("Getting status of non existing box with ID `%s'\n", box_id);
        err = isds_CheckDataBox(ctx, box_id, &box_status);
        if (err)
            printf("isds_CheckDataBox() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        else {
            printf("isds_CheckDataBox() succeeded: status = ");
            print_DbState(box_status);
        }
    }


    /* Send message */
    {
        struct isds_message message;
        memset(&message, 0, sizeof(message));

        struct isds_envelope envelope;
        memset(&envelope, 0, sizeof(envelope));
        message.envelope = &envelope;
        envelope.dbIDRecipient = recipient;
        long int dmSenderOrgUnitNum = 42;
        envelope.dmSenderOrgUnitNum = &dmSenderOrgUnitNum;
        envelope.dmAnnotation = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
       /*     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"*/
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
       
        struct isds_document main_document;
        memset(&main_document, 0, sizeof(main_document));
        main_document.data = "Hello World!";
        main_document.data_length = strlen(main_document.data) + 1;
        main_document.dmMimeType = "text/plain";
        /* XXX: This should fail */
        main_document.dmFileMetaType = FILEMETATYPE_ENCLOSURE;
        /* Server implementation demands dmFileDescr to be valid file name */
        main_document.dmFileDescr = "Standard text.txt";

        struct isds_list documents = {
            .data = &main_document,
            .next = NULL,
            .destructor = NULL
        };
        message.documents = &documents;


        printf("Sending message to box ID `%s'\n",
                message.envelope->dbIDRecipient);
        err = isds_send_message(ctx, &message);
        if (err)
            printf("isds_send_message() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        else {
            printf("isds_send_message() succeeded: assigned message ID = %s\n",
                message.envelope->dmID);
        }

        free(message.envelope->dmID);
    }



    err = isds_logout(ctx);
    if (err) {
        printf("isds_logout() failed: %s\n", isds_strerror(err));
    }

    printf("Ping after logout should fail\n");
    err = isds_ping(ctx);
    if (err) {
        printf("isds_ping() failed: %s\n", isds_strerror(err));
    } else {
        printf("isds_ping() succeeded\n");
    }

    printf("Sending bogus request after logout\n");
    err = isds_bogus_request(ctx);
    if (err) {
        printf("isds_bogus_request() failed: %s: %s\n", isds_strerror(err),
                isds_long_message(ctx));
    } else {
        printf("isds_bogus_request() succeeded\n");
    }

    err = isds_ctx_free(&ctx);
    if (err) {
        printf("isds_ctx_free() failed: %s\n", isds_strerror(err));
    }


    err = isds_cleanup();
    if (err) {
        printf("isds_cleanup() failed: %s\n", isds_strerror(err));
    }

    exit (EXIT_SUCCESS);
}
