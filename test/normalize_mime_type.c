#include "test.h"
#include "crypto.h"
#include <string.h>


static int test_normalize(const char *input, const char *expected) {
    const char *output = isds_normalize_mime_type(input);

    if (!expected) {
        if (!output) {
            PASS_TEST;
        } else  {
            FAIL_TEST("isds_normalize_mime_type(%s) returned=%s, expected=%s",
                    input, output, expected);
        }
    } else if (!output) {
        FAIL_TEST("isds_normalize_mime_type(%s) returned=%s, expected=%s",
                input, output, expected);
    }

    if (strcmp(expected, output)) {
            FAIL_TEST("isds_normalize_mime_type(%s) returned=%s, expected=%s",
                    input, output, expected);
    }

    PASS_TEST;
}


int main(int argc, char **argv) {
    INIT_TEST("normalize_mime_type");

    TEST("NULL is idempotent", test_normalize, NULL, NULL);
    TEST("X-Invalid is idempotent", test_normalize, "X-Invalid", "X-Invalid");
    TEST("pdf", test_normalize, "pdf", "application/pdf");
    TEST("xml", test_normalize, "xml", "application/xml");
    TEST("fo", test_normalize, "fo",
            "application/vnd.software602.filler.xml+form");
    TEST("zfo", test_normalize, "zfo",
            "application/vnd.software602.filler.xml+zip+form");
    TEST("html", test_normalize, "html", "text/html");
    TEST("htm", test_normalize, "htm", "text/html");
    TEST("odt", test_normalize, "odt",
            "application/vnd.oasis.opendocument.text");
    TEST("ods", test_normalize, "ods",
            "application/vnd.oasis.opendocument.spreadsheet");
    TEST("odp", test_normalize, "odp",
            "application/vnd.oasis.opendocument.presentation");
    TEST("txt", test_normalize, "txt", "text/plain");
    TEST("rtf", test_normalize, "rtf", "application/rtf");
    TEST("doc", test_normalize, "doc", "application/msword");
    TEST("xls", test_normalize, "xls", "application/vnd.ms-excel");
    TEST("ppt", test_normalize, "ppt", "application/vnd.ms-powerpoint");
    TEST("jpg", test_normalize, "jpg", "image/jpeg");
    TEST("jpeg", test_normalize, "jpeg", "image/jpeg");
    TEST("jfif", test_normalize, "jfif", "image/jpeg");
    TEST("png", test_normalize, "png", "image/png");
    TEST("tiff", test_normalize, "tiff", "image/tiff");
    TEST("gif", test_normalize, "gif", "image/gif");
    TEST("mpeg1", test_normalize, "mpeg1", "video/mpeg");
    TEST("mpeg2", test_normalize, "mpeg2", "video/mpeg2");
    TEST("wav", test_normalize, "wav", "audio/x-wav");
    TEST("mp2", test_normalize, "mp2", "audio/mpeg");
    TEST("mp3", test_normalize, "mp3", "audio/mpeg");
    /*TEST("isdoc", test_normalize, "isdoc", "");
    TEST("isdocx", test_normalize, "isdocx", "");
    TEST("cer", test_normalize, "cer", "");
    TEST("crt", test_normalize, "crt", "");
    TEST("der", test_normalize, "der", "");
    TEST("pk7", test_normalize, "pk7", "");
    TEST("p7b", test_normalize, "p7b", "");
    TEST("p7c", test_normalize, "p7c", "");
    TEST("p7f", test_normalize, "p7f", "");
    TEST("p7m", test_normalize, "p7m", "");
    TEST("p7s", test_normalize, "p7s", "");
    TEST("tst", test_normalize, "tst", "");*/

    SUM_TEST();
}
