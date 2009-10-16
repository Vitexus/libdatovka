#ifndef __UTILS_H__
#define __UTILS_H__

/* Concatenate two strings into newly allocated buffer.
 * You must free() them, when you don't need it anymore. */
char *astrcat(const char *first, const char *second);

#endif
