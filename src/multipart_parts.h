
#ifndef __ISDS_MULTIPART_PARTS_H__
#define __ISDS_MULTIPART_PARTS_H__

#include <stddef.h> /* size_t */

struct multipart_intermediate; /* Forward declaration. */

struct multipart_part {
	char *content_id;
	char *content_type;
	char *transfer_encoding;

	char *data;
	size_t data_size;

	int invalid_content_id;
	int invalid_content_type;
	int invalid_transfer_encoding;

	struct multipart_part *next; /* Used only in lists. */
};

struct multipart_parts {
	char *expected_root_content_id; /* Expected content ID without opening and closing pointy brackets.*/

	struct multipart_part *processed; /* Currently being processed. */

	struct multipart_part *root; /* Points to root. */
	struct multipart_part *last; /* Points to last entry. */
	struct multipart_part *part_list; /* List of non-root entries. */
};

struct multipart_part *multipart_part_create(void);

/* Ignores next pointer. */
void multipart_part_free(struct multipart_part *mpart);

/* Frees the entire list. */
void multipart_part_deep_free(struct multipart_part *mpart);

void multipart_part_clear(struct multipart_part *mpart);

struct multipart_parts *multipart_parts_create(void);

int multipart_parts_set_root_content_id(struct multipart_parts *mparts, const char *cid);

struct multipart_part *multipart_parts_find_part(struct multipart_parts *mparts, const char *cid);

void multipart_parts_free(struct multipart_parts *mparts);

void on_hfld_and_hval_read(struct multipart_intermediate *mp_ctx);

void on_content_read(struct multipart_intermediate *mp_ctx);

#endif /* __ISDS_MULTIPART_PARTS_H__ */
