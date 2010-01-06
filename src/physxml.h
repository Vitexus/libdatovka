#ifndef __ISDS_PHYSXML_H__
#define __ISDS_PHYSXML_H__

#include "isds.h"

#define PHYSXML_ELEMENT_SEPARATOR "|"
#define PHYSXML_NS_SEPARATOR ">"

/* Check for expat compile-time configuration */
isds_error init_expat(void);

/* Locate element specified by element path in XML stream.
 * TODO: Support other encodings than UTF-8
 * @document is XML documuent as bitstream
 * @length is size of @docuement in bytes. Zero length is forbidden.
 * @path is special path (e.g. "|html|head|title",
 * quallified element names are specified as
 * NSURI '>' LOCALNAME, ommit NSURI and '>' separator if no namespace
 * should be addressed (i.e. use only locale name)
 * You can use PHYSXML_ELEMENT_SEPARATOR and PHYSXML_NS_SEPARATOR string
 * macros.
 * @start outputs start of the element location in @document (inclusive,
 * counts from 0)
 * @end outputs end of element (inclusive, counts from 0)
 * @return 0 if element found */
isds_error find_element_boundary(void *document, size_t length,
        char *path, size_t *start, size_t *end);
#endif
