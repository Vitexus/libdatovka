#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <stdio.h>
/*#include <locale.h>*/
#include <time.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <isds.h>

char url[] = "https://www.czebox.cz/DS/";
char username[] = "jrfh7i";
/*char password[] = "Ab123456";*/
char password[] = "Ac123456";


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

void print_DbType(const long int *type) {
    if (!type) printf("NULL\n");
    else
        switch(*type) {
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
            default: printf("<unknown type %ld>\n", *type);
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
        for (int i = 0; i < hash->length; i++) {
            if (i > 0) printf(":");
            printf("%02x", ((uint8_t *)(hash->value))[i]);
        }

    printf("\n");
}


void print_raw_type(isds_raw_type type) {
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


void print_bool(const _Bool *boolean) {
    printf("%s\n", (!boolean) ? "NULL" : ((*boolean)? "true" : "false") );
}


void print_longint(const long int *number) {
    if (!number) printf("NULL\n");
    else printf("%ld\n", *number);
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
    
    printf("\tdbEffectiveOVM = ");
    print_bool(info->dbEffectiveOVM);

    printf("\tdbOpenAddressing = ");
    print_bool(info->dbOpenAddressing);

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


void print_event_type(const isds_event_type *type) {
    if (!type) {
        printf("NULL");
        return;
    }
    switch (*type) {
        case EVENT_UKNOWN: printf("UNKNOWN\n"); break;
        case EVENT_ACCEPTED_BY_RECIPIENT:
                           printf("ACCEPTED_BY_RECIPIENT\n"); break;
        case EVENT_ACCEPTED_BY_FICTION:
                           printf("DELIVERED_BY_FICTION\n"); break;
        case EVENT_UNDELIVERABLE:
                           printf("UNDELIVERABLE\n"); break;
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
            case MESSAGESTATE_RECEIVED: printf("RECEIVED\n"); break;
            case MESSAGESTATE_READ: printf("READ\n"); break;
            case MESSAGESTATE_UNDELIVERABLE: printf("UNDELIVERABLE\n"); break;
            case MESSAGESTATE_REMOVED: printf("REMOVED\n"); break;
            case MESSAGESTATE_IN_SAFE: printf("IN_SAFE\n"); break;
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
        }
    }
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
        fprintf(stderr, "File `%s' has negative size: %jd\n", file,
                file_info.st_size);
        close(*fd);
        return -1;
    }
    *length = file_info.st_size;

    *buffer = mmap(NULL, *length, PROT_READ, MAP_PRIVATE, *fd, 0);
    if (*buffer == MAP_FAILED) {
        fprintf(stderr, "%s: Could not map file to memmory: %s\n", file,
                strerror(errno));
        close(*fd);
        return -1;
    }

    return 0;
}


int munmap_file(int fd, void *buffer, size_t length) {
    int err = 0;
    long int page_size = sysconf(_SC_PAGE_SIZE);
    size_t pages = (length % page_size) ?
        ((length / page_size) + 1) * page_size:
        length;

    err = munmap(buffer, pages);
    if (err) {
        fprintf(stderr, "Could not unmap memory at %p and length %zu: %s\n",
                buffer, pages, strerror(errno));
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
