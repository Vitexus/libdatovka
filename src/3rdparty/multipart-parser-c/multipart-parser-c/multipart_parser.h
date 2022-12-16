/* Based on node-formidable by Felix Geisendörfer
 * Igor Afonov - afonov@gmail.com - 2012
 * Karel Slany - karel.slany@nic.cz - 2022
 * MIT License - http://www.opensource.org/licenses/mit-license.php
 */
#ifndef _multipart_parser_h
#define _multipart_parser_h

#ifdef __cplusplus
extern "C"
{
#endif

#include <ctype.h>
#include <stdlib.h>

typedef struct multipart_parser multipart_parser;
typedef struct multipart_parser_settings multipart_parser_settings;
typedef struct multipart_parser_state multipart_parser_state;

typedef int (*multipart_data_cb) (multipart_parser*, const char *at, size_t length);
typedef int (*multipart_notify_cb) (multipart_parser*);

struct multipart_parser_settings {
  multipart_data_cb on_header_field;
  multipart_data_cb on_header_value;
  multipart_data_cb on_part_data;

  multipart_notify_cb on_part_data_begin;
  multipart_notify_cb on_header_field_end;
  multipart_notify_cb on_header_value_end;
  multipart_notify_cb on_headers_complete;
  multipart_notify_cb on_part_data_end;
  multipart_notify_cb on_body_end;
};

/*
 * Creates and intialises the parser context.
 * @boundary The boundary string as specified in the HTTP header
 *     (i.e. without the leading tho hyphens ("--") which occur in the actual
 *     multipart message). Argument must not be NULL.
 * @settings Parser settings. Argument must not be NULL.
 * @return Pointer to allocated parser instance.
 */
multipart_parser *multipart_parser_init
    (const char *boundary, const multipart_parser_settings* settings);

void multipart_parser_free(multipart_parser* p);

size_t multipart_parser_execute(multipart_parser* p, const char *buf, size_t len);

void multipart_parser_set_data(multipart_parser* p, void* data);
void * multipart_parser_get_data(multipart_parser* p);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
