
#include <ctype.h> /* tolower() */
#include <stdlib.h> /* free(), malloc(), realloc() */
#include <string.h> /* memcpy() */

#include "compiler.h" /* _hidden */
#include "utils_memory.h"

_hidden int dbuf_init(struct dbuf *dbuf)
{
	if (UNLIKELY(NULL == dbuf)) {
		return -1;
	}

	dbuf->data = NULL;
	dbuf->len = 0;

	return 0;
}

_hidden int dbuf_append(struct dbuf *dbuf, const void *data, size_t len)
{
	if (UNLIKELY(NULL == dbuf)) {
		return -1;
	}

	if (UNLIKELY((NULL == data) && (0 != len))) {
		return -1;
	}

	if (UNLIKELY(0 == len)) {
		return 0;
	}

	/* Resize. */
	size_t new_len = dbuf->len + len;
	void *new_data = realloc(dbuf->data, new_len);
	if (UNLIKELY(NULL == new_data)) {
		return -1;
	}

	/* Copy data. */
	memcpy((char *)new_data + dbuf->len, data, len);

	dbuf->data = new_data;
	dbuf->len = new_len;
	return 0;
}

#define _strncpy_lowercase(dest, src, len) \
{ \
	for (size_t i = 0; i < (len); ++i) { \
		((char *)(dest))[i] = tolower(((char *)(src))[i]); \
	} \
}

_hidden int dbuf_append_lowercase(struct dbuf *dbuf, const void *data, size_t len)
{
	if (UNLIKELY(NULL == dbuf)) {
		return -1;
	}

	if (UNLIKELY((NULL == data) && (0 != len))) {
		return -1;
	}

	if (UNLIKELY(0 == len)) {
		return 0;
	}

	/* Resize. */
	size_t new_len = dbuf->len + len;
	void *new_data = realloc(dbuf->data, new_len);
	if (UNLIKELY(NULL == new_data)) {
		return -1;
	}

	_strncpy_lowercase(((char *)new_data) + dbuf->len, data, len);

	dbuf->data = new_data;
	dbuf->len = new_len;
	return 0;
}

_hidden int dbuf_append_char(struct dbuf *dbuf, char ch)
{
	if (UNLIKELY(NULL == dbuf)) {
		return -1;
	}

	/* Resize. */
	size_t new_len = dbuf->len + 1;
	void *new_data = realloc(dbuf->data, new_len);
	if (UNLIKELY(NULL == new_data)) {
		return -1;
	}

	((char *)new_data)[dbuf->len] = ch;

	dbuf->data = new_data;
	dbuf->len = new_len;
	return 0;
}

_hidden int dbuf_move(struct dbuf *dest, struct dbuf *src)
{
	if (UNLIKELY((NULL == dest) || (NULL == src))) {
		return -1;
	}

	*dest = *src;
	src->data = NULL;
	src->len = 0;

	return 0;
}

_hidden int dbuf_take(struct dbuf *dbuf, void **data, size_t *len)
{
	if (UNLIKELY((NULL == dbuf) || (NULL == data))) {
		return -1;
	}

	*data = dbuf->data; dbuf->data = NULL;
	if (NULL != len) {
		*len = dbuf->len;
	}
	dbuf->len = 0;

	return 0;
}

_hidden void dbuf_free_content(struct dbuf *dbuf)
{
	if (UNLIKELY(NULL == dbuf)) {
		return;
	}

	free(dbuf->data); dbuf->data = NULL;
	dbuf->len = 0;
}

_hidden int dbuf_res_init(struct dbuf_res *dbuf, size_t max_size)
{
	if (UNLIKELY(NULL == dbuf)) {
		return -1;
	}

	if (UNLIKELY(0 == max_size)) {
		dbuf->data = NULL;
		dbuf->used = 0;
		dbuf->max_size = max_size;
		return 0;
	}

	dbuf->data = malloc(max_size);
	if (NULL != dbuf->data) {
		dbuf->used = 0;
		dbuf->max_size = max_size;
		return 0;
	} else {
		dbuf->used = 0;
		dbuf->max_size = 0;
		return -1;
	}
}

_hidden void dbuf_res_empty(struct dbuf_res *dbuf)
{
	if (UNLIKELY(NULL == dbuf)) {
		return;
	}

	if (NULL != dbuf->data) {
		if (dbuf->max_size > 0) {
			/* Null-terminated string. */
			*(char *)dbuf->data = '\0';
		}
		dbuf->used = 0;
	}
}

_hidden int dbuf_res_resize(struct dbuf_res *dbuf, size_t new_max_size)
{
	if (UNLIKELY(NULL == dbuf)) {
		return -1;
	}

	if (UNLIKELY(0 == new_max_size)) {
		free(dbuf->data); dbuf->data = NULL;
		dbuf->used = 0;
		dbuf->max_size = new_max_size;
		return 0;
	}

	void *new_buf = realloc(dbuf->data, new_max_size);
	if (UNLIKELY(NULL == new_buf)) {
		/* Nothing changed. */
		return -1;
	}
	dbuf->data = new_buf;
	if (dbuf->used > new_max_size) {
		dbuf->used = new_max_size;
	}
	dbuf->max_size = new_max_size;
	return 0;
}

_hidden int dbuf_res_append(struct dbuf_res *dbuf, const void *data, size_t len)
{
	if (UNLIKELY(NULL == dbuf)) {
		return -1;
	}

	if (UNLIKELY((NULL == data) && (0 != len))) {
		return -1;
	}

	if (UNLIKELY(0 == len)) {
		return 0;
	}

	/* Resize if needed. */
	if ((dbuf->used + len) >= dbuf->max_size) { /* Leave at least one byte unused. */
		size_t i = ((dbuf->used + len) / BUF_INCREMENT) + 1; /* Leave at least one byte unused. */
		if (UNLIKELY(0 != dbuf_res_resize(dbuf, i * BUF_INCREMENT))) {
			return -1;
		}
	}

	/* Copy data. */
	memcpy((char *)dbuf->data + dbuf->used, data, len);
	dbuf->used += len;
	return 0;
}

_hidden int dbuf_res_append_2(struct dbuf_res *dbuf, size_t increment, const void *data, size_t len)
{
	if (UNLIKELY(NULL == dbuf)) {
		return -1;
	}

	if (UNLIKELY((NULL == data) && (0 != len))) {
		return -1;
	}

	if (UNLIKELY(0 == len)) {
		return 0;
	}

	/* Resize if needed. */
	if ((dbuf->used + len) >= dbuf->max_size) { /* Leave at least one byte unused. */
		size_t i = ((dbuf->used + len) / increment) + 1; /* Leave at least one byte unused. */
		if (UNLIKELY(0 != dbuf_res_resize(dbuf, i * increment))) {
			return -1;
		}
	}

	/* Copy data. */
	memcpy((char *)dbuf->data + dbuf->used, data, len);
	dbuf->used += len;
	return 0;
}

_hidden int dbuf_res_append_lowercase(struct dbuf_res *dbuf, const void *data, size_t len)
{
	if (UNLIKELY(NULL == dbuf)) {
		return -1;
	}

	if (UNLIKELY((NULL == data) && (0 != len))) {
		return -1;
	}

	if (UNLIKELY(0 == len)) {
		return 0;
	}

	/* Resize if needed. */
	if ((dbuf->used + len) >= dbuf->max_size) { /* Leave at least one byte unused. */
		size_t i = ((dbuf->used + len) / BUF_INCREMENT) + 1; /* Leave at least one byte unused. */
		if (UNLIKELY(0 != dbuf_res_resize(dbuf, i * BUF_INCREMENT))) {
			return -1;
		}
	}

	_strncpy_lowercase(((char *)dbuf->data) + dbuf->used, data, len);

	dbuf->used += len;
	return 0;
}

_hidden int dbuf_res_append_char(struct dbuf_res *dbuf, char ch)
{
	if (UNLIKELY(NULL == dbuf)) {
		return -1;
	}

	/* Resize if really needed. */
	if (dbuf->used == dbuf->max_size) {
		if (UNLIKELY(0 != dbuf_res_resize(dbuf, dbuf->used + BUF_INCREMENT))) {
			return -1;
		}
	}

	((char *)dbuf->data)[dbuf->used] = ch;
	++dbuf->used;
	return 0;
}

_hidden int dbuf_res_move(struct dbuf_res *dest, struct dbuf_res *src)
{
	if (UNLIKELY((NULL == dest) || (NULL == src))) {
		return -1;
	}

	*dest = *src;
	src->data = NULL;
	src->used = 0;
	src->max_size = 0;

	return 0;
}

_hidden int dbuf_res_take(struct dbuf_res *dbuf, void **data, size_t *len)
{
	if (UNLIKELY((NULL == dbuf) || (NULL == data))) {
		return -1;
	}

	/* Resize. */
	void *new_buf = realloc(dbuf->data, dbuf->used);
	if (NULL != new_buf) {
		dbuf->data = new_buf;
		dbuf->max_size = dbuf->used;
	}
	/* If resizing fails then just continue. */

	*data = dbuf->data; dbuf->data = NULL;
	if (NULL != len) {
		*len = dbuf->used;
	}
	dbuf->used = 0;
	dbuf->max_size = 0;

	return 0;
}

_hidden void dbuf_res_free_content(struct dbuf_res *dbuf)
{
	if (UNLIKELY(NULL == dbuf)) {
		return;
	}

	free(dbuf->data); dbuf->data = NULL;
	dbuf->used = 0;
	dbuf->max_size = 0;
}
