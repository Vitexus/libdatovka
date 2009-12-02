#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <stdio.h>
/*#include <locale.h>*/
#include <time.h>
#include <string.h>
#include <isds.h>

char url[] = "https://www.czebox.cz/DS/";
char username[] = "jrfh7i";
char password[] = "Ab123456";


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


void print_DbOwnerInfo(const struct isds_DbOwnerInfo *info) {
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


void print_timeval(const struct timeval *time) {
    struct tm broken;
    char buffer[128];

    if (!time) {
        printf("NULL\n");
        return;
    }
    
    if (!localtime_r(&(time->tv_sec), &broken)) goto error;
    if (!strftime(buffer, sizeof(buffer)/sizeof(char), "%c", &broken))
        goto error;
    printf("%s, %ld us\n", buffer, time->tv_usec);
    return;

error:
    printf("<Error while formating>\n>");
    return;
}


void print_envelope(const struct isds_envelope *envelope) {
    printf("\tenvelope = ");

    if (!envelope) {
        printf("NULL\n");
        return;
    }
    printf("{\n");

    printf("\t\tdmID = %s\n", envelope->dmID);

    printf("\t\tdmOrdinal = ");
    if (!envelope->dmOrdinal) printf("NULL\n");
    else printf("%lu\n", *(envelope->dmOrdinal));

    printf("\t\tdmMessageStatus = ");
    if (!envelope->dmMessageStatus) printf("NULL\n");
    else
        switch(*(envelope->dmMessageStatus)) {
            case MESSAGESTATE_SENT: printf("SENT\n"); break;
            case MESSAGESTATE_STAMPED: printf("STAMPED\n"); break;
            case MESSAGESTATE_INFECTED: printf("INFECTED\n"); break;
            case MESSAGESTATE_DELIVERED: printf("DELIVERED\n"); break;
            case MESSAGESTATE_SUBSTITUTED: printf("SUBSTITUTED\n"); break;
            case MESSAGESTATE_RECIEVED: printf("RECIEVED\n"); break;
            case MESSAGESTATE_READ: printf("READ\n"); break;
            case MESSAGESTATE_UNDELIVERABLE: printf("UNDELIVERABLE\n"); break;
            case MESSAGESTATE_REMOVED: printf("REMOVED\n"); break;
            default: printf("<unknown type %d>\n",
                             *(envelope->dmMessageStatus));
        }

    printf("\t\tdmAttachmentSize = ");
    if (!envelope->dmAttachmentSize) printf("NULL\n");
    else printf("%lu kB\n", *(envelope->dmAttachmentSize));

    printf("\t\tdmDeliveryTime = ");
    print_timeval(envelope->dmDeliveryTime);

    printf("\t\tdmAcceptanceTime = ");
    print_timeval(envelope->dmAcceptanceTime);

    printf("\t}\n");
}


void print_message(const struct isds_message *message) {
    printf("message = ");

    if (!message) {
        printf("NULL\n");
        return;
    }

    printf("{\n");

    print_envelope(message->envelope);

    printf("}\n");
}


