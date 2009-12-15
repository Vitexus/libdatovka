#ifndef __COMMON_H__
#define __COMMON_H__

#include <isds.h>

extern char url[];
extern char username[];
extern char password[];

void print_DbState(const long int state);
void print_DbOwnerInfo(const struct isds_DbOwnerInfo *info);
void print_hash(const struct isds_hash *hash);
void print_envelope(const struct isds_envelope *envelope);
void print_message(const struct isds_message *message);

#endif
