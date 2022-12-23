
#ifndef __ISDS_MULTIPART_INTERMEDIATE_H__
#define __ISDS_MULTIPART_INTERMEDIATE_H__

#include <stddef.h> /* size_t */

#include "multipart_parser.h"
#include "utils_memory.h"

/*
 * Intermediate multipart data.
 * The structure just collected complete header names, values ant content.
 */
struct multipart_intermediate {
	struct dbuf_res last_hfld; /* Last header field (header name) in lower case. */
	struct dbuf_res last_hval; /* Last header value. */
	struct dbuf_res content; /* Last content (data). */

	void *data; /* Additional user-defined data to be passed into following callbacks. */

	void (*on_hfld_and_hval_read)(struct multipart_intermediate *ctx);
	void (*on_content_read)(struct multipart_intermediate *ctx);

	struct multipart_parser *parser; /* Multipart parser. */
	struct multipart_parser_settings callbacks; /* Multipart parser callbacks. */
};

/* Create a new instance with specified initial buffer size. */
struct multipart_intermediate *multipart_intermediate_create(size_t buf_size);

/* Initialise intermediate section of the multipart parser. */
int multipart_intermediate_init_parser(struct multipart_intermediate *interm, const char *boundary);

/* Execute the parser on provided data. */
size_t multipart_intermediate_execute(struct multipart_intermediate *interm, const char *buf, size_t len);

/* Doesn't free the data portion. User must free this memory if necessary. */
void multipart_intermediate_free(struct multipart_intermediate *interm);

#endif /* __ISDS_MULTIPART_INTERMEDIATE_H__ */
