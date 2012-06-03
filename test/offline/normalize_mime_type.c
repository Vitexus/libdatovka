#include "../test.h"
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

    TEST("cer", test_normalize, "cer", "application/x-x509-ca-cert");
    TEST("crt", test_normalize, "crt", "application/x-x509-ca-cert");
    TEST("der", test_normalize, "der", "application/x-x509-ca-cert");
    TEST("doc", test_normalize, "doc", "application/msword");
    TEST("docx", test_normalize, "docx",
    "application/vnd.openxmlformats-officedocument.wordprocessingml.document");
    TEST("dbf", test_normalize, "dbf", "application/octet-stream")
    TEST("prj", test_normalize, "prj", "application/octet-stream")
    TEST("qix", test_normalize, "qix", "application/octet-stream")
    TEST("sbx", test_normalize, "sbx", "application/octet-stream")
    TEST("shp", test_normalize, "shp", "application/octet-stream")
    TEST("shx", test_normalize, "shx", "application/octet-stream")
    TEST("dgn", test_normalize, "dgn", "application/octet-stream")
    TEST("dwg", test_normalize, "dwg", "image/vnd.dwg")
    TEST("edi", test_normalize, "edi", "application/edifact")
    TEST("fo", test_normalize, "fo",
            "application/vnd.software602.filler.form+xml");
    TEST("gfs", test_normalize, "gfs", "application/xml")
    TEST("gml", test_normalize, "gml", "application/xml")
    TEST("gif", test_normalize, "gif", "image/gif");
    TEST("htm", test_normalize, "htm", "text/html");
    TEST("html", test_normalize, "html", "text/html");
    TEST("isdoc", test_normalize, "isdoc", "text/isdoc");
    TEST("isdocx", test_normalize, "isdocx", "text/isdocx");
    TEST("jfif", test_normalize, "jfif", "image/jpeg");
    TEST("jpeg", test_normalize, "jpeg", "image/jpeg");
    TEST("jpg", test_normalize, "jpg", "image/jpeg");
    TEST("mpeg", test_normalize, "mpeg", "video/mpeg");
    TEST("mpeg1", test_normalize, "mpeg1", "video/mpeg");
    TEST("mpeg2", test_normalize, "mpeg2", "video/mpeg");
    TEST("mpg", test_normalize, "mpg", "video/mpeg");
    TEST("mp2", test_normalize, "mp2", "audio/mpeg");
    TEST("mp3", test_normalize, "mp3", "audio/mpeg");
    TEST("odp", test_normalize, "odp",
            "application/vnd.oasis.opendocument.presentation");
    TEST("ods", test_normalize, "ods",
            "application/vnd.oasis.opendocument.spreadsheet");
    TEST("odt", test_normalize, "odt",
            "application/vnd.oasis.opendocument.text");
    TEST("p7b", test_normalize, "p7b", "application/pkcs7-certificates");
    TEST("p7c", test_normalize, "p7c", "application/pkcs7-mime");
    TEST("p7f", test_normalize, "p7f", "application/pkcs7-signature");
    TEST("p7m", test_normalize, "p7m", "application/pkcs7-mime");
    TEST("p7s", test_normalize, "p7s", "application/pkcs7-signature");
    TEST("pdf", test_normalize, "pdf", "application/pdf");
    TEST("pk7", test_normalize, "pk7", "application/pkcs7-mime");
    TEST("png", test_normalize, "png", "image/png");
    TEST("ppt", test_normalize, "ppt", "application/vnd.ms-powerpoint");
    TEST("pptx", test_normalize, "pptx",
"application/vnd.openxmlformats-officedocument.presentationml.presentation");
    TEST("rtf", test_normalize, "rtf", "application/rtf");
    TEST("tif", test_normalize, "tif", "image/tiff");
    TEST("tiff", test_normalize, "tiff", "image/tiff");
    TEST("tsr", test_normalize, "tsr", "application/timestamp-reply");
    TEST("tst", test_normalize, "tst", "application/timestamp-reply");
    TEST("txt", test_normalize, "txt", "text/plain");
    TEST("wav", test_normalize, "wav", "audio/wav");
    TEST("xls", test_normalize, "xls", "application/vnd.ms-excel");
    TEST("xlsx", test_normalize, "xlsx",
        "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet");
    TEST("xml", test_normalize, "xml", "application/xml");
    TEST("xsd", test_normalize, "xsd", "application/xml");
    TEST("zfo", test_normalize, "zfo",
            "application/vnd.software602.filler.form-xml-zip");

    SUM_TEST();
}
