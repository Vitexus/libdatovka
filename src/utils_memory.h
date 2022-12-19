#ifndef __ISDS_UTILS_MEMORY_H__
#define __ISDS_UTILS_MEMORY_H__

#include <stddef.h> /* size_t */

#define BUF_INCREMENT 4096

/* Data buffer with no reserve. */
struct dbuf {
	void *data;
	size_t len;
};

/* Data buffer with additional memory reserved. */
struct dbuf_res {
	void *data;
	size_t used;
	size_t max_size;
};

int dbuf_init(struct dbuf *dbuf);

int dbuf_append(struct dbuf *dbuf, const void *data, size_t len);

int dbuf_append_lowercase(struct dbuf *dbuf, const void *data, size_t len);

int dbuf_append_char(struct dbuf *dbuf, char ch);

/* Take the content. */
void *dbuf_take(struct dbuf *dbuf);

void dbuf_free_content(struct dbuf *dbuf);

int dbuf_res_init(struct dbuf_res *dbuf, size_t max_size);

/* Just clear the content, don't reallocate or free memory. */
void dbuf_res_empty(struct dbuf_res *dbuf);

int dbuf_res_resize(struct dbuf_res *dbuf, size_t new_max_size);

int dbuf_res_append(struct dbuf_res *dbuf, const void *data, size_t len);

int dbuf_res_append_lowercase(struct dbuf_res *dbuf, const void *data, size_t len);

int dbuf_res_append_char(struct dbuf_res *dbuf, char ch);

/* Just free the memory held by the data in the buffer. */
void dbuf_res_free_content(struct dbuf_res *dbuf);

#endif /* __ISDS_UTILS_MEMORY_H__ */
