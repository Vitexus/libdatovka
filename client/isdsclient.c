#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <time.h>
#include <isds.h>

char url[] = "https://www.czebox.cz/DS/";
char username[] = "jrfh7i";
char password[] = "Ab123456";
struct isds_ctx *ctx;

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
    else printf("%ld\n", *(info->dbState));
    
    printf("\tdbEffectiveOVM = %s\n",
            !info->dbEffectiveOVM ? "NULL" :
                (*(info->dbEffectiveOVM)? "true" : "false"));

    printf("}\n");

}

int main(int argc, char **argv) {
    isds_error err;
    
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
        struct isds_DbOwnerInfo *db_owner_info = NULL;

        err = isds_GetOwnerInfoFromLogin(ctx, &db_owner_info);
        if (err) {
            printf("isds_GetOwnerInfoFromLogin() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        } else {
            printf("isds_GetOwnerInfoFromLogin() succeeded\n");
        }
        print_DbOwnerInfo(db_owner_info);

        isds_DbOwnerInfo_free(&db_owner_info);
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
