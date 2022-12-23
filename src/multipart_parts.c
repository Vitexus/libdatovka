
#include <stdio.h>
#include <stdlib.h> /* free(), malloc() */
#include <string.h> /* memcpy(), memset(), strcmp() */

#include "compiler.h"
#include "multipart_intermediate.h"
#include "multipart_parts.h"

static
char *_strdup(const char *s)
{
	if (UNLIKELY(NULL == s)) {
		return NULL;
	}

	size_t len = strlen(s);
	char *str = malloc(len + 1);
	if (UNLIKELY(NULL == str)) {
		return NULL;
	}

	memcpy(str, s, len + 1);
	return str;
}

static
_Bool content_id_match(const char *hval, const char *root_cid)
{
	if (UNLIKELY((NULL == hval) || (NULL == root_cid))) {
		return 0;
	}

	const size_t hval_len = strlen(hval);

	return (hval_len > 2)
	    && ((hval_len - 2) == strlen(root_cid))
	    && (hval[0] == '<') && (hval[hval_len - 1] == '>')
	    && (0 == strncmp(hval + 1, root_cid, hval_len - 2));
}

static
void multipart_part_normalise(struct multipart_part *mpart)
{
	if (UNLIKELY(NULL == mpart)) {
		return;
	}

	if (mpart->invalid_content_id) {
		free(mpart->content_id); mpart->content_id = NULL;
	}

	if (mpart->invalid_content_type) {
		free(mpart->content_type); mpart->content_type = NULL;
	}

	if (mpart->invalid_transfer_encoding) {
		free(mpart->transfer_encoding); mpart->transfer_encoding = NULL;
	}
}

_hidden
struct multipart_part *multipart_part_create(void)
{
	struct multipart_part *mpart = malloc(sizeof(*mpart));
	if (UNLIKELY(NULL == mpart)) {
		return NULL;
	}

	memset(mpart, 0, sizeof(*mpart));

	return mpart;
}

_hidden
void multipart_part_free(struct multipart_part *mpart)
{
	if (UNLIKELY(NULL == mpart)) {
		return;
	}

	free(mpart->content_id);
	free(mpart->content_type);
	free(mpart->transfer_encoding);
	free(mpart->data);

	free(mpart);
}

_hidden
void multipart_part_deep_free(struct multipart_part *mpart)
{
	while (NULL != mpart) {
		struct multipart_part *next = mpart->next;
		multipart_part_free(mpart);
		mpart = next;
	}
}

_hidden
void multipart_part_clear(struct multipart_part *mpart)
{
	if (UNLIKELY(NULL == mpart)) {
		return;
	}

	free(mpart->content_id);
	free(mpart->content_type);
	free(mpart->transfer_encoding);
	free(mpart->data);

	memset(mpart, 0, sizeof(*mpart));
}

_hidden
struct multipart_parts *multipart_parts_create(void)
{
	struct multipart_parts *mparts = malloc(sizeof(*mparts));
	if (UNLIKELY(NULL == mparts)) {
		return NULL;
	}

	memset(mparts, 0, sizeof(*mparts));

	return mparts;
}

_hidden
int multipart_parts_set_root_content_id(struct multipart_parts *mparts, const char *cid)
{
	if (UNLIKELY(NULL == mparts)) {
		return -1;
	}

	free(mparts->expected_root_content_id); mparts->expected_root_content_id = NULL;

	if (NULL != cid) {
		mparts->expected_root_content_id = _strdup(cid);
		if (UNLIKELY(NULL == mparts->expected_root_content_id)) {
			return -1;
		}
	}

	return 0;
}

_hidden
struct multipart_part *multipart_parts_find_part(struct multipart_parts *mparts, const char *cid)
{
	if (UNLIKELY(NULL == mparts)) {
		return NULL;
	}

	struct multipart_part *part = mparts->part_list;

	while (NULL != part) {
		if (UNLIKELY((NULL == cid) && (NULL == part->content_id))) {
			return part;
		}
		if (0 == strcmp(cid, part->content_id)) {
			return part;
		}

		part = part->next;
	}

	return NULL;
}

_hidden
void multipart_parts_free(struct multipart_parts *mparts)
{
	if (UNLIKELY(NULL == mparts)) {
		return;
	}

	free(mparts->expected_root_content_id);

	multipart_part_free(mparts->processed);
	multipart_part_deep_free(mparts->part_list);

	free(mparts);
}

_hidden
void on_hfld_and_hval_read(struct multipart_intermediate *mp_ctx)
{
	if (UNLIKELY(NULL == mp_ctx)) {
		return;
	}
	struct multipart_parts *mparts = (struct multipart_parts *)mp_ctx->data;
	if (UNLIKELY(NULL == mparts)) {
		return;
	}
	char *hfld = (char *)mp_ctx->last_hfld.data;
	char *hval = (char *)mp_ctx->last_hval.data;

	if (NULL == mparts->processed) {
		mparts->processed = multipart_part_create();
		/* TODO -- Error handling. */
	}
	struct multipart_part *mpart = mparts->processed;

	/* Copy header values. */
	if (0 == strcmp(hfld, "content-id")) {
		if (NULL == mpart->content_id) {
			mpart->content_id = _strdup(hval);
			/* TODO -- Error handling. */
		} else {
			/* Already set. */
			mpart->invalid_content_id = 1;
		}
	} else if (0 == strcmp(hfld, "content-type")) {
		if (NULL == mpart->content_type) {
			mpart->content_type = _strdup(hval);
			/* TODO -- Error handling. */
		} else {
			/* Already set. */
			mpart->invalid_content_type = 1;
		}
	} else if (0 == strcmp(hfld, "content-transfer-encoding")) {
		if (NULL == mpart->transfer_encoding) {
			mpart->transfer_encoding = _strdup(hval);
			/* TODO -- Error handling. */
		} else {
			/* Already set. */
			mpart->invalid_transfer_encoding = 1;
		}
	}
}

_hidden
void on_content_read(struct multipart_intermediate *mp_ctx)
{
	if (UNLIKELY(NULL == mp_ctx)) {
		return;
	}
	struct multipart_parts *mparts = (struct multipart_parts *)mp_ctx->data;
	if (UNLIKELY(NULL == mparts)) {
		return;
	}
	struct dbuf_res *content = &mp_ctx->content;

	if (NULL == mparts->processed) {
		mparts->processed = multipart_part_create();
		/* TODO -- Error handling. */
	}

	{
		struct multipart_part *mpart = mparts->processed;

		/* Copy data content. */
		if ((NULL != content->data)) {
			mpart->data = malloc(content->used);
			if (NULL != mpart->data) {
				memcpy(mpart->data, content->data, content->used);
				mpart->data_size = content->used;
			}
		}

		/* Normalise. */
		multipart_part_normalise(mpart);

		if ((NULL == mpart->content_id) || ('\0' == *mpart->content_id)) {
			multipart_part_free(mparts->processed); mparts->processed = NULL;
			return;
		}
	}

	/* Append to list. */
	if (NULL != mparts->part_list) {
		mparts->last->next = mparts->processed;
	} else {
		mparts->part_list = mparts->processed;
	}
	mparts->last = mparts->processed;
	mparts->processed = NULL;

	if (0 == strcmp(mparts->last->content_id, mparts->expected_root_content_id)) {
		if (NULL == mparts->root) {
			/* Move to root. */
			mparts->root = mparts->last;
		} else {
			/* TODO -- Error handling, root already set. */
		}
	}
}
