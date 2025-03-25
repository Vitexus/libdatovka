
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h> /* free(), malloc() */
#include <string.h> /* memset() */

#include "compiler.h"
#include "multipart_intermediate.h"

static
void _intermediate_log(const char *func, const char *file, int line, const char * format, ...)
{
#ifdef DEBUG_MULTIPART_INTERMEDIATE
	va_list args;
	va_start(args, format);

	fprintf(stderr, "[HTTP_MULTIPART_INTERMEDIATE:%s()] %s:%d: ", func, file, line);
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");

	va_end(args);
#else /* !DEBUG_MULTIPART_INTERMEDIATE */
	(void)func;
	(void)file;
	(void)line;
	(void)format;
#endif /* DEBUG_MULTIPART_INTERMEDIATE */
}

/* Use intermediate_log(format, ...) */
#define intermediate_log(...) \
  _intermediate_log(__func__, __FILE__, __LINE__, __VA_ARGS__)

/* Convert portion of header field (header name) into lower-case and append to buffer. */
static
int read_header_name(struct multipart_parser *p, const char *at, size_t length)
{
	if (UNLIKELY(NULL == p)) {
		return -1;
	}
	struct multipart_intermediate *interm =
	    (struct multipart_intermediate *)multipart_parser_get_data(p);
	if (UNLIKELY(NULL == interm)) {
		return -1;
	}
	struct dbuf_res *dbuf = &interm->last_hfld;

	//intermediate_log("%.*s", (int)length, at);

	if (0 == dbuf_res_append_lowercase(dbuf, at, length)) {
		return 0;
	} else {
		return -1;
	}
}

/* Append portion of header value to buffer. */
static
int read_header_value(struct multipart_parser *p, const char *at, size_t length)
{
	if (UNLIKELY(NULL == p)) {
		return -1;
	}
	struct multipart_intermediate *interm =
	    (struct multipart_intermediate *)multipart_parser_get_data(p);
	if (UNLIKELY(NULL == interm)) {
		return -1;
	}
	struct dbuf_res *dbuf = &interm->last_hval;

	//intermediate_log("%.*s", (int)length, at);

	if (0 == dbuf_res_append_2(dbuf, BUF_RES_INCREMENT, at, length)) {
		return 0;
	} else {
		return -1;
	}
}

/* Append portion of content into buffer. */
static
int read_part_data(struct multipart_parser *p, const char *at, size_t length)
{
	if (UNLIKELY(NULL == p)) {
		return -1;
	}
	struct multipart_intermediate *interm =
	    (struct multipart_intermediate *)multipart_parser_get_data(p);
	if (UNLIKELY(NULL == interm)) {
		return -1;
	}
	struct dbuf_res *dbuf = &interm->content;

	//intermediate_log("%.*s", (int)length, at);

	if (0 == dbuf_res_append_2(dbuf, BUF_RES_INCREMENT, at, length)) {
		interm->have_unfinished_content = 1;
		return 0;
	} else {
		return -1;
	}
}

/* Called when multipart section begins. */
static
int part_data_begin(struct multipart_parser *p)
{
	if (UNLIKELY(NULL == p)) {
		return -1;
	}
	struct multipart_intermediate *interm =
	    (struct multipart_intermediate *)multipart_parser_get_data(p);
	if (UNLIKELY(NULL == interm)) {
		return -1;
	}
	struct dbuf_res *dbuf = &interm->content;

	intermediate_log("");

	/* Clear any stored content. */
	dbuf->used = 0;
	return 0;
}

/* Called when complete header field (header name) has been read. */
static
int header_field_end(struct multipart_parser *p)
{
	if (UNLIKELY(NULL == p)) {
		return -1;
	}
	struct multipart_intermediate *interm =
	    (struct multipart_intermediate *)multipart_parser_get_data(p);
	if (UNLIKELY(NULL == interm)) {
		return -1;
	}
	struct dbuf_res *dbuf = &interm->last_hfld;
	dbuf_res_append_char(dbuf, '\0'); /* Null-terminated string now. */

	intermediate_log("'%.*s'", (int)dbuf->used, dbuf->data);

	return 0;
}

/*
 * Called when complete header value has been read.
 * Calls on_hfld_and_hval_read().
 * Clears header buffers.
 */
static
int header_value_end(struct multipart_parser *p)
{
	if (UNLIKELY(NULL == p)) {
		return -1;
	}
	struct multipart_intermediate *interm =
	    (struct multipart_intermediate *)multipart_parser_get_data(p);
	if (UNLIKELY(NULL == interm)) {
		return -1;
	}
	struct dbuf_res *dbuf = &interm->last_hval;
	dbuf_res_append_char(dbuf, '\0'); /* Null-terminated string now. */

	intermediate_log("'%.*s'", (int)dbuf->used, dbuf->data);

	if (NULL != interm->on_hfld_and_hval_read) {
		interm->on_hfld_and_hval_read(interm);
	}

	/* Clear header buffers. */
	dbuf_res_empty(&interm->last_hfld);
	dbuf_res_empty(&interm->last_hval);
	return 0;
}

/*
 * Called on start of content.
 * Clears content buffer.
 */
static
int headers_complete(struct multipart_parser *p)
{
	if (UNLIKELY(NULL == p)) {
		return -1;
	}
	struct multipart_intermediate *interm =
	    (struct multipart_intermediate *)multipart_parser_get_data(p);
	if (UNLIKELY(NULL == interm)) {
		return -1;
	}
	struct dbuf_res *dbuf = &interm->content;

	intermediate_log("");

	dbuf->used = 0;
	return 0;
}

/*
 * Called on end of content.
 * Calls on_content_read().
 * Clears content buffer.
 */
static
int part_data_end(struct multipart_parser *p)
{
	if (UNLIKELY(NULL == p)) {
		return -1;
	}
	struct multipart_intermediate *interm =
	    (struct multipart_intermediate *)multipart_parser_get_data(p);
	if (UNLIKELY(NULL == interm)) {
		return -1;
	}
	struct dbuf_res *dbuf = &interm->content;

	intermediate_log("'%.*s'", (int)dbuf->used, dbuf->data);

	if (NULL != interm->on_content_read) {
		interm->on_content_read(interm);
	}

	dbuf->used = 0;
	interm->have_unfinished_content = 0;
	return 0;
}

/* Called when all parts complete. */
static
int body_end(struct multipart_parser *p)
{
	(void)p;

	intermediate_log("");

	return 0;
}

_hidden
struct multipart_intermediate *multipart_intermediate_create(size_t buf_size)
{
	struct multipart_intermediate *interm = malloc(sizeof(*interm));
	if (UNLIKELY(NULL == interm)) {
		return NULL;
	}
	memset(interm, 0, sizeof(*interm));

	if (UNLIKELY(0 != dbuf_res_init(&interm->last_hfld, buf_size))) {
		goto fail;
	}
	if (UNLIKELY(0 != dbuf_res_init(&interm->last_hval, buf_size))) {
		goto fail;
	}
	if (UNLIKELY(0 != dbuf_res_init(&interm->content, buf_size))) {
		goto fail;
	}

	return interm;

fail:
	multipart_intermediate_free(interm);
	return NULL;
}

_hidden
int multipart_intermediate_init_parser(struct multipart_intermediate *interm, const char *boundary)
{
	if (UNLIKELY((NULL == interm) ||
	    (NULL == boundary) || ('\0' == *boundary))) {
		return -1;
	}

	interm->callbacks.on_header_field = read_header_name;
	interm->callbacks.on_header_value = read_header_value;
	interm->callbacks.on_part_data = read_part_data;

	interm->callbacks.on_part_data_begin = part_data_begin;
	interm->callbacks.on_header_field_end = header_field_end;
	interm->callbacks.on_header_value_end = header_value_end;
	interm->callbacks.on_headers_complete = headers_complete;
	interm->callbacks.on_part_data_end = part_data_end;
	interm->callbacks.on_body_end = body_end;

	interm->parser = multipart_parser_init(boundary, &interm->callbacks);
	if (UNLIKELY(NULL == interm->parser)) {
		memset(&interm->callbacks, 0, sizeof(interm->callbacks));
		return -1;
	}

	multipart_parser_set_data(interm->parser, interm);
	return 0;
}

_hidden
size_t multipart_intermediate_execute(struct multipart_intermediate *interm, const char *buf, size_t len)
{
	if (UNLIKELY((NULL == interm) || (NULL == interm->parser))) {
		return 0;
	}

	return multipart_parser_execute(interm->parser, buf, len);
}

_hidden
int multipart_intermediate_finish(struct multipart_intermediate *interm)
{
	if (UNLIKELY((NULL == interm) || (NULL == interm->parser))) {
		return 0;
	}

	if (interm->have_unfinished_content) {
		part_data_end(interm->parser);
		return 1;
	}
	return 0;
}

_hidden
void multipart_intermediate_free(struct multipart_intermediate *interm)
{
	if (UNLIKELY(NULL == interm)) {
		return;
	}

	dbuf_res_free_content(&interm->last_hfld);
	dbuf_res_free_content(&interm->last_hval);
	dbuf_res_free_content(&interm->content);

	multipart_parser_free(interm->parser);

	free(interm);
}
