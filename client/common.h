#ifndef __COMMON_H__
#define __COMMON_H__

#include <isds.h>

extern char url[];
extern char username[];
extern char password[];

void print_bool(const _Bool *boolean);
void print_longint(const long int *number);
void print_DbState(const long int state);
void print_date(const struct tm *date);
void print_DbOwnerInfo(const struct isds_DbOwnerInfo *info);
void print_DbUserInfo(const struct isds_DbUserInfo *info);
void print_timeval(const struct timeval *time);
void print_hash(const struct isds_hash *hash);
void print_envelope(const struct isds_envelope *envelope);
void print_message(const struct isds_message *message);
void print_copies(const struct isds_list *copies);

void compare_hashes(const struct isds_hash *hash1,
        const struct isds_hash *hash2);
int progressbar(double upload_total, double upload_current,
    double download_total, double download_current,
    void *data);

int mmap_file(const char *file, int *fd, void **buffer, size_t *length);
int munmap_file(int fd, void *buffer, size_t length);
int save_data(const char *message, const void *data, const size_t length);

#endif
