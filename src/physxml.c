#define _POSIX_SOURCE   /* For strtok_r */
#include "isds_priv.h"
#include "utils.h"

/*#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>*/
#include <string.h>
#include <expat.h>
#include <inttypes.h>

#define PHYSXML_ELEMENT_SEPARATOR "|"
#define PHYSXML_NS_SEPARATOR ">"
#define NS_CHAR_SEPARATOR '>'

struct expat_data {
    XML_Parser parser;
    const XML_Char **elements;  /* NULL terminated array of elements */
    _Bool found;
    size_t *start;
    size_t *end;
    int depth;              /* Current parser depth, root element is 0 */
    int element_depth;      /* elements[element_depth] we are in,
                                -1 if we are not in any (root mismatch)*/
};


/* Check for expat compile-time configuration */
_hidden isds_error expat_init(void) {
    XML_Expat_Version current;
    const int min_major = 1;
    const int min_minor = 95;
    const int min_micro = 8;
    const XML_Feature *features; /* Static array stored in expat BSS */
    _Bool ns_supported = 0;

 /*
 * Max(XML_Size) <= Max(size_t)
 * XML_Char is char, not a wchar_t
 * XML_UNICODE is undefined (i.e. strings in UTF-8)
 * */

    /* Check minimal expat version */
    current = XML_ExpatVersionInfo();
    if ( (current.major < min_major) ||
            (current.major == min_major && current.minor < min_minor) ||
            (current.major == min_major && current.minor == min_minor &&
                current.micro < min_micro) ) {
        isds_log(ILF_ISDS, ILL_CRIT,
                _("Minimal %d.%d.%d Expat version required. "
                "Current version is %d.%d.%d\n"),
                min_major, min_minor, min_micro,
                current.major, current.minor, current.micro);
        return IE_ERROR;
    }

    /* XML_Char must be char, not a wchar_t */
    features = XML_GetFeatureList();
    while (features->feature != XML_FEATURE_END) {
        switch (features->feature) {
            case XML_FEATURE_UNICODE_WCHAR_T:
            case XML_FEATURE_UNICODE:
                isds_log(ILF_ISDS, ILL_CRIT,
                        _("Expat compiled with UTF-16 (wide) characters\n"));
                return IE_ERROR;
                break;
            case XML_FEATURE_SIZEOF_XML_CHAR:
                if (features->value != sizeof(char)) {
                    isds_log(ILF_ISDS, ILL_CRIT,
                            "Expat compiled with XML_Chars incompatible "
                            "with chars\n");
                    return IE_ERROR;
                }
                break;
            case XML_FEATURE_NS:
                ns_supported = 1;
            default:
                break;
        }
        features++;
    }

    if (!ns_supported) {
        isds_log(ILF_ISDS, ILL_CRIT,
                _("Expat not compiled with name space support\n"));
        return IE_ERROR;
    }

    return IE_SUCCESS;
}


/* Breaks element path address into NULL terminated array of elements in
 * preserved order. Zeroth array element will be first path element.
 * @path  element address, content will be damaged
 * @return array of elements, NULL in case of error */
static const XML_Char **path2elements(XML_Char *path) {
    const XML_Char **elements = NULL;
    XML_Char *tmp_path;
    char *saveptr = NULL;
    XML_Char *element;
    unsigned int depth = 0;

    if (!path) return NULL;

    elements = malloc(sizeof(elements[0]) * (strlen(path) + 1));
    if (!elements) return NULL;

    elements[0] = NULL;

    tmp_path = path;
    while ((element = (XML_Char *) strtok_r(tmp_path,
                    PHYSXML_ELEMENT_SEPARATOR, &saveptr))) {
        tmp_path = NULL;
        elements[depth++] = element;
    }

    elements[depth] = NULL;
    return elements;
}


/* Examine start and empty element tag.
 * @name is expanded name */
static void XMLCALL element_start(void *userData, const XML_Char *name,
        const XML_Char **atts) {
    struct expat_data *data = (struct expat_data *) userData;
    data->depth++;

    const XML_Index index = XML_GetCurrentByteIndex(data->parser);
    /*const int count = XML_GetCurrentByteCount(data->parser);*/
    /* XXX: Because document length is stored as size_t, index always fits
     * size_t. */
    const size_t boundary = index; 

    /*printf("Start: name=%s, depth=%zd, offset=%#jx "
            "count=%u => boundary=%#zx\n",
            name, data->depth, (uintmax_t)index, count, boundary); */

    if ((!data->found) &&
            (data->depth == data->element_depth + 1) &&
            (!strcmp(data->elements[data->element_depth + 1], name))) {
        data->element_depth++;

        /*printf("! Start tag for element `%s' found\n",
                data->elements[data->element_depth]);*/
        
        if (!data->elements[data->element_depth + 1]) {
            data->found = 1;
            *data->start = boundary;
        }
    }
}


/* Examine end and empty element tag.
 * @name is expanded name */
static void XMLCALL element_end(void *userData, const XML_Char *name) {

    struct expat_data *data = (struct expat_data *) userData;
    enum XML_Status xerr;

    const XML_Index index = (uintmax_t) XML_GetCurrentByteIndex(data->parser);
    const int count = XML_GetCurrentByteCount(data->parser);
    /* XXX: Because document length is stored as size_t, index + count always
     * fits size_t. */
    const size_t boundary = index + count - 1; 

    /*printf("End:   name=%s, depth=%zd, offset=%#jx "
            "count=%u => boundary=%#zx\n",
            name, data->depth, (uintmax_t)index, count, boundary);*/

    if (data->element_depth == data->depth) {
        if (data->found) {
            /*printf("! End tag for element `%s' found\n",
                    data->elements[data->element_depth]);*/
            *data->end = boundary;

            /* Here we can stop parser
             * XXX: requires Expat 1.95.8 */
            xerr = XML_StopParser(data->parser, XML_FALSE);
            if (xerr != XML_STATUS_OK) {
                PANIC(_("Error while stopping parser\n"));
            }

        }
        data->element_depth--;
    }

    data->depth--;
}


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
_hidden isds_error find_element_boundary(void *document, size_t length,
        char *path, size_t *start, size_t *end) {
    
    XML_Parser parser;
    enum XML_Status xerr;
    struct expat_data user_data;

    if (!document || !path || !start || !end || length <= 0)
        return IE_INVAL;

    /* Parse XPath */
    user_data.elements = path2elements(path);
    if (!user_data.elements) return IE_NOMEM;

    /* No element means whole document */
    if (!user_data.elements[0]) {
        free(user_data.elements);
        *start = 0;
        *end = length - 1;
        return IE_SUCCESS;
    }

    /* Create parser*/
    parser = XML_ParserCreateNS(NULL, NS_CHAR_SEPARATOR);

    XML_SetStartElementHandler(parser, element_start);
    XML_SetEndElementHandler(parser, element_end);
    
    user_data.parser = parser;
    user_data.found = 0;
    user_data.start = start;
    user_data.end = end;
    user_data.depth = -1;
    user_data.element_depth = -1;
    XML_SetUserData(parser, &user_data);

    /* Parse it */
    xerr = XML_Parse(parser, (const char *) document, length, 1);
    if (xerr != XML_STATUS_OK &&
            !( (xerr == XML_STATUS_ERROR &&
                    XML_GetErrorCode(parser) == XML_ERROR_ABORTED))) {
        free(user_data.elements);
        isds_log(ILF_ISDS, ILL_CRIT, _("XML_Parse failed\n"));
        return IE_ERROR;
    }
    free(user_data.elements);

    XML_ParserFree(parser);
    if (user_data.found) return IE_SUCCESS;
    else return IE_NOEXIST;
}

