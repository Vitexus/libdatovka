#include "../test.h"
#include "isds.c"

#if HAVE_LIBCURL
static int test_filemetatype2string_must_fail(const isds_FileMetaType type) {
    xmlChar *string;

    string = (xmlChar *) isds_FileMetaType2string(type);
    if (string) 
        FAIL_TEST("conversion from isds_FileMetaType to string did not fail");

    PASS_TEST;
}
#endif

static int test_string2filemetatype_must_fail(const xmlChar *string) {
    isds_error err;
    isds_FileMetaType new_type;

    err = string2isds_FileMetaType((xmlChar *)string, &new_type);
    if (!err)
        FAIL_TEST("conversion from string to isds_FileMetaType did not fail");

    PASS_TEST;
}

static int test_filemetatype(const isds_FileMetaType type,
        const xmlChar *string) {
    isds_error err;
    isds_FileMetaType new_type;
#if HAVE_LIBCURL
    xmlChar *new_string;

    new_string = (xmlChar *) isds_FileMetaType2string(type);
    if (!new_string) 
        FAIL_TEST("conversion from isds_FileMetaType to string failed");
    if (xmlStrcmp(string, new_string))
        FAIL_TEST("conversion from isds_FileMetaType %zd to string returned "
                "unexpected value: got=`%s', expected=`%s'",
                type, new_string, string);
#endif /* HAVE_LIBCURL */

    err = string2isds_FileMetaType(string, &new_type);
    if (err)
        FAIL_TEST("conversion from string to isds_FileMetaTyoe failed");
    if (type != new_type)
        FAIL_TEST("conversion from string `%s' to isds_FileMetaType returned "
                "unexpected valued: got=%zd, expected=%zd",
                string, new_type, type);

    PASS_TEST;
}

int main(void) {
    INIT_TEST("isds_FileMetaType conversion");
    
    isds_FileMetaType types[] =  {
        FILEMETATYPE_MAIN,              /* Main document */
        FILEMETATYPE_ENCLOSURE,         /* Appendix */
        FILEMETATYPE_SIGNATURE,         /* Digital signature of other document */
        FILEMETATYPE_META               /* XML document for ESS (electronic */
    };
    const xmlChar *strings[] = {
        BAD_CAST "main",
        BAD_CAST "enclosure",
        BAD_CAST "signature",
        BAD_CAST "meta"
    };

    for (size_t i = 0; i < sizeof(types)/sizeof(types[0]); i++)
        TEST(strings[i], test_filemetatype, types[i], strings[i]);

#if HAVE_LIBCURL
    TEST("1234", test_filemetatype2string_must_fail, 1234); 
#endif
    
    TEST("X-Invalid_Type", test_string2filemetatype_must_fail,
            BAD_CAST "X-Invalid_Type");

    SUM_TEST();
}
