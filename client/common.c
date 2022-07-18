#define _XOPEN_SOURCE 700
#include <stdlib.h>
#include <stdio.h>
/*#include <locale.h>*/
#include <time.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <libdatovka/isds.h>

char credentials_file[] = "../test_credentials";

static int read_config(char **line, int order) {
    FILE *file;
    size_t length = 0;
    char *eol;

    if (!line) return -1;
    free(*line);
    *line = NULL;

    file = fopen(credentials_file, "r");
    if (!file) {
        fprintf(stderr, "Could open %s\n", credentials_file);
        return -1;
    }

    for (int i = 0; i < order; i++) {
        if (-1 == getline(line, &length, file)) {
            fprintf(stderr, "Could not read line #%d from %s: ",
                    i + 1, credentials_file);
            if (ferror(file))
                fprintf(stderr, "error occurred\n");
            else if (feof(file))
                fprintf(stderr, "end of file reached\n");
            else
                fprintf(stderr, "I don't know why\n");
            fclose(file);
            free(*line);
            *line = NULL;
            return -1;
        }
    }

    fclose(file);

    eol = strpbrk(*line, "\r\n");
    if (eol) *eol = '\0';

    return 0;
}

const char *username(void) {
    static char *username;

    if (!username) {
        username = getenv("ISDS_USERNAME");
        if (!username)
            read_config(&username, 1);
    }
    return username;
}

const char *password(void) {
    static char *password;

    if (!password) {
        password = getenv("ISDS_PASSWORD");
        if (!password)
            read_config(&password, 2);
    }
    return password;
}

void print_DbState(const long int state) {
    switch(state) {
        case DBSTATE_ACCESSIBLE: printf("ACCESSIBLE\n"); break;
        case DBSTATE_TEMP_INACCESSIBLE: printf("TEMP_INACCESSIBLE\n"); break;
        case DBSTATE_NOT_YET_ACCESSIBLE: printf("NOT_YET_ACCESSIBLE\n"); break;
        case DBSTATE_PERM_INACCESSIBLE: printf("PERM_INACCESSIBLE\n"); break;
        case DBSTATE_REMOVED: printf("REMOVED\n"); break;
        case DBSTATE_TEMP_INACCESSIBLE_LAW: printf("DBSTATE_TEMP_INACCESSIBLE_LAW"); break;
        default: printf("<unknown state %ld>\n", state);
    }
}

void print_DbType(const long int *type) {
    if (!type) printf("NULL\n");
    else
        switch(*type) {
            case DBTYPE_SYSTEM: printf("SYSTEM\n"); break;
            case DBTYPE_FO: printf("FO\n"); break;
            case DBTYPE_PFO: printf("PFO\n"); break;
            case DBTYPE_PFO_ADVOK: printf("PFO_ADVOK\n"); break;
            case DBTYPE_PFO_DANPOR: printf("PFO_DAPOR\n"); break;
            case DBTYPE_PFO_INSSPR: printf("PFO_INSSPR\n"); break;
            case DBTYPE_PFO_AUDITOR: printf("PFO_AUDITOR\n"); break;
            case DBTYPE_PFO_ZNALEC: printf("PFO_ZNALEC\n"); break;
            case DBTYPE_PFO_TLUMOCNIK: printf("PFO_TLUMOCNIK\n"); break;
            case DBTYPE_PO: printf("PO\n"); break;
            case DBTYPE_PO_ZAK: printf("PO_ZAK\n"); break;
            case DBTYPE_PO_REQ: printf("PO_REQ\n"); break;
            case DBTYPE_OVM: printf("OVM\n"); break;
            case DBTYPE_OVM_NOTAR: printf("OVM_NOTAR\n"); break;
            case DBTYPE_OVM_EXEKUT: printf("OVM_EXEKUT\n"); break;
            case DBTYPE_OVM_REQ: printf("OVM_REQ\n"); break;
            case DBTYPE_OVM_FO: printf("OVM_FO\n"); break;
            case DBTYPE_OVM_PFO: printf("OVM_PFO\n"); break;
            case DBTYPE_OVM_PO: printf("OVM_PO\n"); break;
            default: printf("<unknown type %ld>\n", *type);
        }
}


void print_UserType(const long int *type) {
    if (!type) printf("NULL\n");
    else
        switch(*type) {
            case USERTYPE_PRIMARY: printf("PRIMARY\n"); break;
            case USERTYPE_ENTRUSTED: printf("ENTRUSTED\n"); break;
            case USERTYPE_ADMINISTRATOR: printf("ADMINISTRATOR\n"); break;
            case USERTYPE_OFFICIAL: printf("OFFICIAL\n"); break;
            case USERTYPE_OFFICIAL_CERT: printf("OFFICIAL_CERT\n"); break;
            case USERTYPE_LIQUIDATOR: printf("LIQUIDATOR\n"); break;
            case USERTYPE_RECEIVER: printf("RECEIVER\n"); break;
            case USERTYPE_GUARDIAN: printf("GUARDIAN\n"); break;
            default: printf("<unknown type %ld>\n", *type);
        }
}


void print_sender_type(const isds_sender_type *type) {
    if (!type) printf("NULL\n");
    else
        switch(*type) {
            case SENDERTYPE_PRIMARY: printf("PRIMARY\n"); break;
            case SENDERTYPE_ENTRUSTED: printf("ENTRUSTED\n"); break;
            case SENDERTYPE_ADMINISTRATOR: printf("ADMINISTRATOR\n"); break;
            case SENDERTYPE_OFFICIAL: printf("OFFICIAL\n"); break;
            case SENDERTYPE_VIRTUAL: printf("VIRTUAL\n"); break;
            case SENDERTYPE_OFFICIAL_CERT: printf("OFFICIAL_CERT\n"); break;
            case SENDERTYPE_LIQUIDATOR: printf("LIQUIDATOR\n"); break;
            case SENDERTYPE_RECEIVER: printf("RECEIVER\n"); break;
            case SENDERTYPE_GUARDIAN: printf("GUARDIAN\n"); break;
            default: printf("<unknown type %u>\n", *type);
        }
}

static
void print_UserPrivils(const long int *privils) {

    const char *priviledges[] = {
        "READ_NON_PERSONAL",
        "READ_ALL",
        "CREATE_DM",
        "VIEW_INFO",
        "SEARCH_DB",
        "OWNER_ADM",
        "READ_VAULT",
        "ERASE_VAULT"
    };
    const int priviledges_count = sizeof(priviledges)/sizeof(priviledges[0]);

    if (!privils) printf("NULL\n");
    else {
        printf("%ld (", *privils);

        for (int i = 0; i < priviledges_count; i++) {
            if (*privils & (1<<i)) {
                printf(
                        ((i + 1) == priviledges_count) ? "%s" : "%s|",
                        priviledges[i]);
            }
        }

        printf(")\n");
    }
}


void print_hash(const struct isds_hash *hash) {
    if (!hash) {
        printf("NULL\n");
        return;
    }

    switch(hash->algorithm) {
        case HASH_ALGORITHM_MD5: printf("MD5 "); break;
        case HASH_ALGORITHM_SHA_1: printf("SHA-1 "); break;
        case HASH_ALGORITHM_SHA_256: printf("SHA-256 "); break;
        case HASH_ALGORITHM_SHA_512: printf("SHA-512 "); break;
        default: printf("<Unknown hash algorithm %d> ", hash->algorithm);
                 break;
    }

    if (!hash->value) printf("<NULL>");
    else
        for (size_t i = 0; i < hash->length; i++) {
            if (i > 0) printf(":");
            printf("%02x", ((uint8_t *)(hash->value))[i]);
        }

    printf("\n");
}


void print_raw_type(const isds_raw_type type) {
    switch(type) {
        case RAWTYPE_INCOMING_MESSAGE:
            printf("INCOMING_MESSAGE\n"); break;
        case RAWTYPE_PLAIN_SIGNED_INCOMING_MESSAGE:
            printf("PLAIN_SIGNED_INCOMING_MESSAGE\n"); break;
        case RAWTYPE_CMS_SIGNED_INCOMING_MESSAGE:
            printf("CMS_SIGNED_INCOMING_MESSAGE\n"); break;
        case RAWTYPE_PLAIN_SIGNED_OUTGOING_MESSAGE:
            printf("PLAIN_SIGNED_OUTGOING_MESSAGE\n"); break;
        case RAWTYPE_CMS_SIGNED_OUTGOING_MESSAGE:
            printf("CMS_SIGNED_OUTGOING_MESSAGE\n"); break;
        case RAWTYPE_DELIVERYINFO:
            printf("DELIVERYINFO\n"); break;
        case RAWTYPE_PLAIN_SIGNED_DELIVERYINFO:
            printf("PLAIN_SIGNED_DELIVERYINFO\n"); break;
        case RAWTYPE_CMS_SIGNED_DELIVERYINFO:
            printf("CMS_SIGNED_DELIVERYINFO\n"); break;
        default:
            printf("<Unknown raw type %d> ", type);
            break;
    }
}

static void print_dmMessageStatus(const isds_message_status *status) {
    if (!status) printf("NULL\n");
    else
        switch(*status) {
            case MESSAGESTATE_SENT: printf("SENT\n"); break;
            case MESSAGESTATE_STAMPED: printf("STAMPED\n"); break;
            case MESSAGESTATE_INFECTED: printf("INFECTED\n"); break;
            case MESSAGESTATE_DELIVERED: printf("DELIVERED\n"); break;
            case MESSAGESTATE_SUBSTITUTED: printf("SUBSTITUTED\n"); break;
            case MESSAGESTATE_RECEIVED: printf("RECEIVED\n"); break;
            case MESSAGESTATE_READ: printf("READ\n"); break;
            case MESSAGESTATE_UNDELIVERABLE: printf("UNDELIVERABLE\n"); break;
            case MESSAGESTATE_REMOVED: printf("REMOVED\n"); break;
            case MESSAGESTATE_IN_VAULT: printf("IN_VAULT\n"); break;
            default: printf("<unknown type %d>\n", *status);
        }
}

void print_bool(const _Bool *boolean) {
    printf("%s\n", (!boolean) ? "NULL" : ((*boolean)? "true" : "false") );
}


void print_longint(const long int *number) {
    if (!number) printf("NULL\n");
    else printf("%ld\n", *number);
}

void print_ulongint(const unsigned long int *number)
{
	if (NULL == number) {
		fputs("NULL\n", stdout);
		return;
	}
	fprintf(stdout, "%lu\n", *number);
}

void print_isds_status(const struct isds_status *status)
{
	const char *db = "db";
	const char *dm = "dm";
	const char *unknown = "?";
	const char *pref = unknown;

	if (NULL == status) {
		fputs("?Status = NULL\n", stdout);
		return;
	}

	switch (status->type) {
	case STAT_DB: pref = db; break;
	case STAT_DM: pref = dm; break;
	default:
	    break;
	}

	fprintf(stdout, "%sStatus = {\n", pref);
	fprintf(stdout, "\t%sStatusCode = %s\n", pref, status->code);
	fprintf(stdout, "\t%sStatusMessage = %s\n", pref, status->message);
	fprintf(stdout, "\t%sStatusRefNumber = %s\n", pref, status->ref_number);
	fputs("}\n", stdout);
}

static
void print_PersonName(const struct isds_PersonName *personName) {
    printf("\tpersonName = ");
    if (!personName) printf("NULL\n");
    else {
        printf("{\n");
        printf("\t\tpnFirstName = %s\n", personName->pnFirstName);
        printf("\t\tpnMiddleName = %s\n", personName->pnMiddleName);
        printf("\t\tpnLastName = %s\n", personName->pnLastName);
        printf("\t\tpnLastNameAtBirth = %s\n", personName->pnLastNameAtBirth);
        printf("\t}\n");
    }
}

static
void print_PersonName2(const struct isds_PersonName2 *personName) {
    printf("\tpersonName = ");
    if (!personName) printf("NULL\n");
    else {
        printf("{\n");
        printf("\t\tpnGivenNames = %s\n", personName->pnGivenNames);
        printf("\t\tpnLastName = %s\n", personName->pnLastName);
        printf("\t}\n");
    }
}


void print_Address(const struct isds_Address *address) {
    printf("\taddress = ");
    if (!address) printf("NULL\n");
    else {
        printf("{\n");
        printf("\t\tadCity = %s\n", address->adCity);
        printf("\t\tadStreet = %s\n", address->adStreet);
        printf("\t\tadNumberInStreet = %s\n", address->adNumberInStreet);
        printf("\t\tadNumberInMunicipality = %s\n",
                address->adNumberInMunicipality);
        printf("\t\tadZipCode = %s\n", address->adZipCode);
        printf("\t\tadState = %s\n", address->adState);
        printf("\t}\n");
    }
}

static
void print_AddressExt2(const struct isds_AddressExt2 *address) {
    printf("\taddress = ");
    if (!address) printf("NULL\n");
    else {
        printf("{\n");
        printf("\t\tadCode = %s\n", address->adCode);
        printf("\t\tadCity = %s\n", address->adCity);
        printf("\t\tadDistrict = %s\n", address->adDistrict);
        printf("\t\tadStreet = %s\n", address->adStreet);
        printf("\t\tadNumberInStreet = %s\n", address->adNumberInStreet);
        printf("\t\tadNumberInMunicipality = %s\n",
                address->adNumberInMunicipality);
        printf("\t\tadZipCode = %s\n", address->adZipCode);
        printf("\t\tadState = %s\n", address->adState);
        printf("\t}\n");
    }
}


void print_date(const struct tm *date) {
    if (!date) printf("NULL\n");
    else printf("%s", asctime(date));
}

static
void print_BirthInfo(const struct isds_BirthInfo *birthInfo) {
    printf("\tbirthInfo = ");
    if (!birthInfo) printf("NULL\n");
    else {
        printf("{\n");

        printf("\t\tbiDate = ");
        print_date(birthInfo->biDate);

        printf("\t\tbiCity = %s\n", birthInfo->biCity);
        printf("\t\tbiCounty = %s\n", birthInfo->biCounty);
        printf("\t\tbiState = %s\n", birthInfo->biState);
        printf("\t}\n");
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
    print_DbType((long int *) (info->dbType));
    printf("\tic = %s\n", info->ic);

    print_PersonName(info->personName);

    printf("\tfirmName = %s\n", info->firmName);

    print_BirthInfo(info->birthInfo);

    print_Address(info->address);

    printf("\tnationality = %s\n", info->nationality);
    printf("\temail = %s\n", info->email);
    printf("\ttelNumber = %s\n", info->telNumber);
    printf("\tidentifier = %s\n", info->identifier);
    printf("\tregistryCode = %s\n", info->registryCode);

    printf("\tdbState = ");
    if (!info->dbState) printf("NULL\n");
    else print_DbState(*(info->dbState));

    printf("\tdbEffectiveOVM = ");
    print_bool(info->dbEffectiveOVM);

    printf("\tdbOpenAddressing = ");
    print_bool(info->dbOpenAddressing);

    printf("}\n");
}


void print_DbOwnerInfoExt2(const struct isds_DbOwnerInfoExt2 *info) {
    printf("DbOwnerInfoExt2 = ");

    if (!info) {
        printf("NULL\n");
        return;
    }

    printf("{\n");
    printf("\tdbID = %s\n", info->dbID);

    printf("\taifoIsds = ");
    print_bool(info->aifoIsds);

    printf("\tdbType = ");
    print_DbType((long int *) (info->dbType));
    printf("\tic = %s\n", info->ic);

    print_PersonName2(info->personName);

    printf("\tfirmName = %s\n", info->firmName);

    print_BirthInfo(info->birthInfo);

    print_AddressExt2(info->address);

    printf("\tnationality = %s\n", info->nationality);

    printf("\tdbIdOVM = %s\n", info->dbIdOVM);

    printf("\tdbState = ");
    if (!info->dbState) printf("NULL\n");
    else print_DbState(*(info->dbState));

    printf("\tdbOpenAddressing = ");
    print_bool(info->dbOpenAddressing);

    printf("\tdbUpperID = %s\n", info->dbUpperID);

    printf("}\n");
}


void print_DbUserInfo(const struct isds_DbUserInfo *info) {
    printf("dbUserInfo = ");

    if (!info) {
        printf("NULL\n");
        return;
    }

    printf("{\n");
    printf("\tuserID = %s\n", info->userID);

    printf("\tuserType = ");
    print_UserType((long int *) (info->userType));

    printf("\tuserPrivils = ");
    print_UserPrivils(info->userPrivils);

    print_PersonName(info->personName);
    print_Address(info->address);

    printf("\tbiDate = ");
    print_date(info->biDate);

    printf("\tic = %s\n", info->ic);
    printf("\tfirmName = %s\n", info->firmName);

    printf("\tcaStreet = %s\n", info->caStreet);
    printf("\tcaCity = %s\n", info->caCity);
    printf("\tcaZipCode = %s\n", info->caZipCode);
    printf("\tcaState = %s\n", info->caState);

    printf("}\n");
}


void print_DbUserInfoExt2(const struct isds_DbUserInfoExt2 *info) {
    printf("dbUserInfoExt2 = ");

    if (!info) {
        printf("NULL\n");
        return;
    }

    printf("{\n");
    printf("\taifoIsds = ");
    print_bool(info->aifoIsds);

    print_PersonName2(info->personName);

    print_AddressExt2(info->address);

    printf("\tbiDate = ");
    print_date(info->biDate);

    printf("\tisdsID = %s\n", info->isdsID);

    printf("\tuserType = ");
    print_UserType((long int *) (info->userType));

    printf("\tuserPrivils = ");
    print_UserPrivils(info->userPrivils);

    printf("\tic = %s\n", info->ic);
    printf("\tfirmName = %s\n", info->firmName);

    printf("\tcaStreet = %s\n", info->caStreet);
    printf("\tcaCity = %s\n", info->caCity);
    printf("\tcaZipCode = %s\n", info->caZipCode);
    printf("\tcaState = %s\n", info->caState);

    printf("}\n");
}

static
void print_vault_type(enum isds_vault_type *type)
{
	if (NULL == type) {
		fputs("NULL\n", stdout);
		return;
	}

	switch (*type) {
	case VAULT_NONE: fputs("VAULT_NONE\n", stdout); break;
	case VAULT_PREPAID: fputs("VAULT_PREPAID\n", stdout); break;
	case VAULT_UNUSED_2: fputs("VAULT_UNUSED_2\n", stdout); break;
	case VAULT_CONTRACTUAL: fputs("VAULT_CONTRACTUAL\n", stdout); break;
	case VAULT_TRIAL: fputs("VAULT_TRIAL\n", stdout); break;
	case VAULT_UNUSED_5: fputs("VAULT_UNUSED_5\n", stdout); break;
	case VAULT_SPECIAL_OFFER: fputs("VAULT_SPECIAL_OFFER\n", stdout); break;
	default: fprintf(stdout, "<unknown value %d>\n", *type);
	}
}

static
void print_vault_payment_status(enum isds_vault_payment_status *status)
{
	if (NULL == status) {
		fputs("NULL\n", stdout);
		return;
	}

	switch (*status) {
	case VAULT_NOT_PAID_YET: fputs("VAULT_NOT_PAID_YET\n", stdout); break;
	case VAULT_PAID_ALREADY: fputs("VAULT_PAID_ALREADY\n", stdout); break;
	default: fprintf(stdout, "<unknown value %d>\n", *status);
	}
}

void print_DTInfoOutput(const struct isds_DTInfoOutput *info)
{
	fputs("DTInfoOutput = ", stdout);

	if (NULL == info) {
		printf("NULL\n");
		return;
	}

	fputs("{\n", stdout);
	fputs("\tactDTType = ", stdout);
	print_vault_type(info->actDTType);

	fputs("\tactDTCapacity = ", stdout);
	print_ulongint(info->actDTCapacity);

	fputs("\tactDTFrom = ", stdout);
	print_date(info->actDTFrom);

	fputs("\tactDTTo = ", stdout);
	print_date(info->actDTTo);

	fputs("\tactDTCapUsed = ", stdout);
	print_ulongint(info->actDTCapUsed);

	fputs("\tfutDTType = ", stdout);
	print_vault_type(info->futDTType);

	fputs("\tfutDTCapacity = ", stdout);
	print_ulongint(info->futDTCapacity);

	fputs("\tfutDTFrom = ", stdout);
	print_date(info->futDTFrom);

	fputs("\tfutDTTo = ", stdout);
	print_date(info->futDTTo);

	fputs("\tfutDTPaid = ", stdout);
	print_vault_payment_status(info->futDTPaid);

	fputs("}\n", stdout);
}

void print_timeval(const struct isds_timeval *time) { // !!!
    struct tm broken;
    char buffer[128];
    time_t seconds_as_time_t;

    if (!time) {
        printf("NULL\n");
        return;
    }

    /* MinGW32 GCC 4.8+ uses 64-bit time_t but time->tv_sec is defined as
     * 32-bit long in Microsoft API. Convert value to the type expected by
     * gmtime_r(). */
    seconds_as_time_t = time->tv_sec;
    if (!localtime_r(&seconds_as_time_t, &broken)) goto error;
    if (!strftime(buffer, sizeof(buffer)/sizeof(char), "%c", &broken))
        goto error;
    printf("%s, %" PRIdMAX " us\n", buffer, (intmax_t)time->tv_usec);
    return;

error:
    printf("<Error while formatting>\n>");
    return;
}


void print_event_type(const isds_event_type *type) {
    if (!type) {
        printf("NULL");
        return;
    }
    switch (*type) {
        case EVENT_UNKNOWN: printf("UNKNOWN\n"); break;
        case EVENT_ENTERED_SYSTEM: printf("ENTERED_SYSTEM\n"); break;
        case EVENT_ACCEPTED_BY_RECIPIENT:
                           printf("ACCEPTED_BY_RECIPIENT\n"); break;
        case EVENT_ACCEPTED_BY_FICTION:
                           printf("ACCEPTED_BY_FICTION\n"); break;
        case EVENT_ACCEPTED_BY_FICTION_NO_USER:
                           printf("ACCEPTED_BY_FICTION_NO_USER\n"); break;
        case EVENT_UNDELIVERABLE:
                           printf("UNDELIVERABLE\n"); break;
        case EVENT_COMMERCIAL_ACCEPTED:
                           printf("COMMERCIAL_ACCEPTED\n"); break;
        case EVENT_DELIVERED:
                           printf("DELIVERED\n"); break;
        case EVENT_PRIMARY_LOGIN:
                           printf("PRIMARY_LOGIN\n"); break;
        case EVENT_ENTRUSTED_LOGIN:
                           printf("ENTRUSTED_LOGIN\n"); break;
        case EVENT_SYSCERT_LOGIN:
                           printf("SYSCERT_LOGIN\n"); break;
        default: printf("<unknown type %d>\n", *type);
    }
}


void print_events(const struct isds_list *events) {
    const struct isds_list *item;
    const struct isds_event *event;

    if (!events) {
        printf("NULL\n");
        return;
    }

    printf("{\n");

    for (item = events; item; item = item->next) {
        event = (struct isds_event *) item->data;
        printf("\t\t\tevent = ");
        if (!event) printf("NULL");
        else {
            printf("{\n");

            printf("\t\t\t\ttype = ");
            print_event_type(event->type);

            printf("\t\t\t\tdescription = %s\n", event->description);

            printf("\t\t\t\ttime = ");
            print_timeval(event->time);

            printf("\t\t\t}\n");
        }
    }

    printf("\t\t}\n");
}


void print_envelope(const struct isds_envelope *envelope) {
    printf("\tenvelope = ");

    if (!envelope) {
        printf("NULL\n");
        return;
    }
    printf("{\n");

    printf("\t\tdmID = %s\n", envelope->dmID);
    printf("\t\tdbIDSender = %s\n", envelope->dbIDSender);
    printf("\t\tdmSender = %s\n", envelope->dmSender);
    printf("\t\tdmSenderAddress = %s\n", envelope->dmSenderAddress);
    printf("\t\tdmSenderType = ");
    print_DbType(envelope->dmSenderType);
    printf("\t\tdmRecipient = %s\n", envelope->dmRecipient);
    printf("\t\tdmRecipientAddress = %s\n", envelope->dmRecipientAddress);
    printf("\t\tdmAmbiguousRecipient = ");
    print_bool(envelope->dmAmbiguousRecipient);
    printf("\t\tdmType = %s\n", envelope->dmType);

    printf("\t\tdmSenderOrgUnit = %s\n", envelope->dmSenderOrgUnit);
    printf("\t\tdmSenderOrgUnitNum = ");
    print_longint(envelope->dmSenderOrgUnitNum);
    printf("\t\tdbIDRecipient = %s\n", envelope->dbIDRecipient);
    printf("\t\tdmRecipientOrgUnit = %s\n", envelope->dmRecipientOrgUnit);
    printf("\t\tdmRecipientOrgUnitNum = ");
    print_longint(envelope->dmRecipientOrgUnitNum);
    printf("\t\tdmToHands = %s\n", envelope->dmToHands);
    printf("\t\tdmAnnotation = %s\n", envelope->dmAnnotation);
    printf("\t\tdmRecipientRefNumber = %s\n", envelope->dmRecipientRefNumber);
    printf("\t\tdmSenderRefNumber = %s\n", envelope->dmSenderRefNumber);
    printf("\t\tdmRecipientIdent = %s\n", envelope->dmRecipientIdent);
    printf("\t\tdmSenderIdent = %s\n", envelope->dmSenderIdent);

    printf("\t\tdmLegalTitleLaw = ");
    print_longint(envelope->dmLegalTitleLaw);
    printf("\t\tdmLegalTitleYear = ");
    print_longint(envelope->dmLegalTitleYear);
    printf("\t\tdmLegalTitleSect = %s\n", envelope->dmLegalTitleSect);
    printf("\t\tdmLegalTitlePar = %s\n", envelope->dmLegalTitlePar);
    printf("\t\tdmLegalTitlePoint = %s\n", envelope->dmLegalTitlePoint);

    printf("\t\tdmPersonalDelivery = ");
    print_bool(envelope->dmPersonalDelivery);
    printf("\t\tdmAllowSubstDelivery = ");
    print_bool(envelope->dmAllowSubstDelivery);
    printf("\t\tdmOVM = ");
    print_bool(envelope->dmOVM);
    printf("\t\tdmPublishOwnID = ");
    print_bool(envelope->dmPublishOwnID);

    printf("\t\tdmOrdinal = ");
    if (!envelope->dmOrdinal) printf("NULL\n");
    else printf("%lu\n", *(envelope->dmOrdinal));

    printf("\t\tdmMessageStatus = ");
    print_dmMessageStatus(envelope->dmMessageStatus);

    printf("\t\tdmAttachmentSize = ");
    if (!envelope->dmAttachmentSize) printf("NULL\n");
    else printf("%lu kB\n", *(envelope->dmAttachmentSize));

    printf("\t\tdmDeliveryTime = ");
    print_timeval(envelope->dmDeliveryTime);

    printf("\t\tdmAcceptanceTime = ");
    print_timeval(envelope->dmAcceptanceTime);

    printf("\t\thash = ");
    print_hash(envelope->hash);

    printf("\t\ttimestamp = %p\n", envelope->timestamp);
    printf("\t\ttimestamp_length = %zu\n", envelope->timestamp_length);

    printf("\t\tevents = ");
    print_events(envelope->events);

    printf("\t}\n");
}


void print_document(const struct isds_document *document) {
    printf("\t\tdocument = ");

    if (!document) {
        printf("NULL\n");
        return;
    }
    printf("\{\n");

    printf("\t\t\tis_xml = %u\n", !!document->is_xml);
    printf("\t\t\txml_node_list = %p\n", document->xml_node_list);

    printf("\t\t\tdata = %p\n", document->data);
    printf("\t\t\tdata_length = %zu\n", document->data_length);
    printf("\t\t\tdmMimeType = %s\n", document->dmMimeType);

    printf("\t\t\tdmFileMetaType = ");
    switch(document->dmFileMetaType) {
        case FILEMETATYPE_MAIN: printf("MAIN\n"); break;
        case FILEMETATYPE_ENCLOSURE: printf("ENCLOSURE\n"); break;
        case FILEMETATYPE_SIGNATURE: printf("SIGNATURE\n"); break;
        case FILEMETATYPE_META: printf("META\n"); break;
        default: printf("<unknown type %d>\n", document->dmFileMetaType);
    }

    printf("\t\t\tdmFileGuid = %s\n", document->dmFileGuid);
    printf("\t\t\tdmUpFileGuid = %s\n", document->dmUpFileGuid);
    printf("\t\t\tdmFileDescr = %s\n", document->dmFileDescr);
    printf("\t\t\tdmFormat = %s\n", document->dmFormat);
    printf("\t\t}\n");
}


void print_documents(const struct isds_list *documents) {
    const struct isds_list *item;

    printf("\tdocuments = ");

    if (!documents) {
        printf("NULL\n");
        return;
    }
    printf("{\n");

    for (item = documents; item; item = item->next) {
        print_document((struct isds_document *) (item->data));
    }

    printf("\t}\n");
}


void print_message(const struct isds_message *message) {
    printf("message = ");

    if (!message) {
        printf("NULL\n");
        return;
    }

    printf("{\n");

    printf("\traw = %p\n", message->raw);
    printf("\traw_length = %zu\n", message->raw_length);
    printf("\traw_type = ");
    print_raw_type(message->raw_type);
    printf("\txml = %p\n", message->xml);
    print_envelope(message->envelope);
    print_documents(message->documents);

    printf("}\n");
}

void print_copies(const struct isds_list *copies) {
    const struct isds_list *item;
    struct isds_message_copy *copy;

    printf("Copies = ");
    if (!copies) {
        printf("<NULL>\n");
        return;
    }

    printf("{\n");
    for (item = copies; item; item = item->next) {
        copy = (struct isds_message_copy *) item->data;
        printf("\tCopy = ");

        if (!copy)
            printf("<NULL>\n");
        else {
            printf("{\n");
            printf("\t\tdbIDRecipient = %s\n", copy->dbIDRecipient);
            printf("\t\tdmRecipientOrgUnit = %s\n", copy->dmRecipientOrgUnit);

            printf("\t\tdmRecipientOrgUnitNum = ");
            if (copy->dmRecipientOrgUnitNum)
                printf("%ld\n", *copy->dmRecipientOrgUnitNum);
            else
                printf("<NULL>\n");
            printf("\t\tdmToHands = %s\n", copy->dmToHands);

            printf("\t\terror = %s\n", isds_strerror(copy->error));
            printf("\t\tdmStatus = %s\n", copy->dmStatus);
            printf("\t\tdmID = %s\n", copy->dmID);
            printf("\t}\n");
        }
    }
    printf("}\n");
}

void print_message_status_change(
        const struct isds_message_status_change *changed_status) {
    printf("message_status_change = ");

    if (!changed_status) {
        printf("NULL\n");
        return;
    }

    printf("{\n");

    printf("\tdmID = %s\n", changed_status->dmID);

    printf("\tdmMessageStatus = ");
    print_dmMessageStatus(changed_status->dmMessageStatus);

    printf("\ttime = ");
    print_timeval(changed_status->time);

    printf("}\n");
}

void print_dmMessageAuthor(struct isds_dmMessageAuthor *author)
{
	printf("dmMessageAuthor = ");

	if (NULL == author) {
		printf("NULL\n");
		return;
	}

	printf("{\n");
	printf("\tuserType = ");
	print_sender_type(author->userType);

	print_PersonName2(author->personName);

	printf("\tbiDate = ");
	print_date(author->biDate);

	printf("\tbiCity = %s\n", author->biCity);
	printf("\tbiCounty = %s\n", author->biCounty);
	printf("\tadCode = %s\n", author->adCode);
	printf("\tfullAddress = %s\n", author->fullAddress);

	printf("\trobIdent = ");
	print_bool(author->robIdent);

	printf("}\n");
}

void compare_hashes(const struct isds_hash *hash1,
        const struct isds_hash *hash2) {
    isds_error err;

    printf("Comparing hashes... ");
    err = isds_hash_cmp(hash1, hash2);
    if (err == IE_SUCCESS)
        printf("Hashes equal\n");
    else if
        (err == IE_NOTEQUAL) printf("Hashes differ\n");
    else
        printf("isds_hash_cmp() failed: %s\n", isds_strerror(err));
}


int progressbar(double upload_total, double upload_current,
    double download_total, double download_current,
    void *data) {

    printf("Progress: upload %0f/%0f, download %0f/%0f, data=%p\n",
            upload_current, upload_total, download_current, download_total,
            data);
    if (data) {
        printf("Aborting transfer...\n");
        return 1;
    }
    return 0;
}


int mmap_file(const char *file, int *fd, void **buffer, size_t *length) {
    struct stat file_info;

    if (!file || !fd || !buffer || !length) return -1;


    *fd = open(file, O_RDONLY);
    if (*fd == -1) {
        fprintf(stderr, "%s: Could not open file: %s\n", file, strerror(errno));
        return -1;
    }

    if (-1 == fstat(*fd, &file_info)) {
        fprintf(stderr, "%s: Could not get file size: %s\n", file,
                strerror(errno));
        close(*fd);
        return -1;
    }
    if (file_info.st_size < 0) {
        fprintf(stderr, "File `%s' has negative size: %" PRIdMAX "\n", file,
                (intmax_t) file_info.st_size);
        close(*fd);
        return -1;
    }
    *length = file_info.st_size;

    if (!*length) {
        /* Empty region cannot be mmapped */
        *buffer = NULL;
    } else {
        *buffer = mmap(NULL, *length, PROT_READ, MAP_PRIVATE, *fd, 0);
        if (*buffer == MAP_FAILED) {
            fprintf(stderr, "%s: Could not map file to memory: %s\n", file,
                    strerror(errno));
            close(*fd);
            return -1;
        }
    }

    return 0;
}


int munmap_file(int fd, void *buffer, size_t length) {
    int err = 0;
    long int page_size = sysconf(_SC_PAGE_SIZE);
    size_t pages = (length % page_size) ?
        ((length / page_size) + 1) * page_size:
        length;

    if (length) {
        err = munmap(buffer, pages);
        if (err) {
            fprintf(stderr,
                    "Could not unmap memory at %p and length %zu: %s\n",
                    buffer, pages, strerror(errno));
        }
    }

    err = close(fd);
    if (err) {
        fprintf(stderr, "Could close file descriptor %d: %s\n", fd,
                strerror(errno));
    }

    return err;
}


static int save_data_to_file(const char *file, const void *data,
        const size_t length) {
    int fd;
    ssize_t written, left = length;

    if (!file) return -1;
    if (length > 0 && !data) return -1;

    fd = open(file, O_WRONLY|O_TRUNC|O_CREAT, 0666);
    if (fd == -1) {
        fprintf(stderr, "%s: Could not open file for writing: %s\n",
                file, strerror(errno));
        return -1;
    }

    printf("Writing %zu bytes to file `%s'...\n", length, file);
    while (left) {
        written = write(fd, data + length - left, left);
        if (written == -1) {
            fprintf(stderr, "%s: Could not save file: %s\n",
                    file, strerror(errno));
            close(fd);
            return -1;
        }
        left-=written;
    }

    if (-1 == close(fd)) {
        fprintf(stderr, "%s: Closing file failed: %s\n",
                file, strerror(errno));
        return -1;
    }

    printf("Done.\n");
    return 0;
}


int save_data(const char *message, const void *data, const size_t length) {
    if (message)
        printf("%s\n", message);
    return save_data_to_file("output", data, length);
}
