#ifndef __ISDS_INTERNAL_TYPES_H__
#define __ISDS_INTERNAL_TYPES_H__

#include <libxml/tree.h> /* xmlNodePtr */

/*
 * Communication request. Consists of XML request data. May also contain
 * portions of binary data that should be transferred in a multi-part request.
 */
struct comm_req {
	const xmlNodePtr request; /*
	                           * XML request content that should be
	                           * transmitted inside a SOAP envelope.
	                           */
	const char *content_id; /* href value of the Include MTOM/XOP element */
	const struct isds_dmFile *dm_file; /* file content */
};

#endif /* __ISDS_INTERNAL_TYPES_H__ */
