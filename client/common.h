#ifndef __COMMON_H__
#define __COMMON_H__

#include <isds.h>

extern char url[];
extern char username[];
extern char password[];

void print_DbState(const long int state);
void print_DbOwnerInfo(struct isds_DbOwnerInfo *info);
void print_envelope(struct isds_envelope *envelope);
void print_message(struct isds_message *message);

#endif
