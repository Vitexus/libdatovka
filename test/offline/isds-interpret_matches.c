#include "../test.h"
#include <stddef.h> /* For ptrdiff_t */
#include "isds.c"

static int compare_lists(const char *label,
        const xmlChar *expected_string, const struct isds_list *expected_list,
        const xmlChar* string, const struct isds_list *list) {
    const struct isds_list *expected_item, *item;
    ptrdiff_t expected_offset, offset;
    int i;

    TEST_POINTER_DUPLICITY(expected_list, list);
    for (i = 1, expected_item = expected_list, item = list;
            NULL != expected_item && NULL != item;
            i++, expected_item = expected_item->next, item = item->next) {
        expected_offset = (xmlChar *)expected_item->data - expected_string;
        offset = (xmlChar *)item->data - string;
        if (expected_offset != offset)
            FAIL_TEST("%d. %s match offsets differ: expected=%td, got=%td",
                    i, label, expected_offset, offset);
    }
    if (NULL != expected_item && NULL == item)
        FAIL_TEST("%s match offsets list is missing %d. item", label, i);
    if (NULL == expected_item && NULL != item)
        FAIL_TEST("%s match offsets list has superfluous %d. item", label, i);

    return 0;
}


static int test_interpret_matches(const xmlChar *input_string,
        isds_error expected_error,
        const xmlChar *expected_string,
        const struct isds_list *expected_starts,
        const struct isds_list *expected_ends,
        struct isds_list **starts,
        struct isds_list **ends) {
    isds_error err;
    xmlChar *string = NULL;

    if (NULL != input_string) {
        string = xmlStrdup(input_string);
        if (NULL == string)
            ABORT_UNIT("Could not duplicate a string");
        TEST_DESTRUCTOR(xmlFree, string);
    }

    err = interpret_matches(string, starts, ends);

    if (expected_error != err)
        FAIL_TEST("interpret_matches() returned wrong error code: "
                "expected=%s, got=%s",
                isds_strerror(expected_error), isds_strerror(err));
    if (err != IE_SUCCESS)
        PASS_TEST;

    TEST_STRING_DUPLICITY((const char *)expected_string, (const char *)string);
    if (NULL != starts) {
        if (compare_lists("start", expected_string, expected_starts,
                    string, *starts))
            return 1;
    }
    if (NULL != ends) {
        if (compare_lists("end", expected_string, expected_ends,
                    string, *ends))
            return 1;
    }

    PASS_TEST;
}


#define START_MARKER "|$*HL_START*$|"
#define END_MARKER "|$*HL_END*$|"

int main(void) {
    xmlChar *input, *expected_output;
    struct isds_list expected_starts = { .data = NULL, .next = NULL };
    struct isds_list expected_ends = { .data = NULL, .next = NULL };
    struct isds_list expected_starts2 = { .data = NULL, .next = NULL };
    struct isds_list expected_ends2 = { .data = NULL, .next = NULL };
    struct isds_list *starts = NULL, *ends = NULL;
    INIT_TEST("interpret_matches string conversion");

    if (isds_init())
        ABORT_UNIT("isds_init() failed\n");

    TEST("NULL start", test_interpret_matches, BAD_CAST NULL,
            IE_INVAL, BAD_CAST NULL, NULL, NULL, NULL, &ends);
    TEST("NULL ends", test_interpret_matches, BAD_CAST NULL,
            IE_INVAL, BAD_CAST NULL, NULL, NULL, &starts, NULL);
    TEST("NULL string", test_interpret_matches, BAD_CAST NULL,
            IE_SUCCESS, BAD_CAST NULL, NULL, NULL, &starts, &ends);
    TEST("Empty string", test_interpret_matches, BAD_CAST "",
            IE_SUCCESS, BAD_CAST "", NULL, NULL, &starts, &ends);

    /* No matches */
    input = BAD_CAST "foo";
    TEST((const char *)input, test_interpret_matches, input,
            IE_SUCCESS, input, NULL, NULL, &starts, &ends);

    /* One match is the whole empty string */
    input = BAD_CAST START_MARKER END_MARKER;
    expected_output = BAD_CAST "";
    expected_starts.data = expected_output;
    expected_ends.data = expected_output;
    TEST((const char *)input, test_interpret_matches, input,
            IE_SUCCESS, expected_output,
            &expected_starts, &expected_ends, &starts, &ends);

    /* One match is the whole string */
    input = BAD_CAST START_MARKER "MATCH" END_MARKER;
    expected_output = BAD_CAST "MATCH";
    expected_starts.data = expected_output;
    expected_ends.data = expected_output + 5;
    TEST((const char *)input, test_interpret_matches, input,
            IE_SUCCESS, expected_output,
            &expected_starts, &expected_ends, &starts, &ends);

    /* One match in the beginning */
    input = BAD_CAST START_MARKER "MATCH" END_MARKER "after";
    expected_output = BAD_CAST "MATCH" "after";
    expected_starts.data = expected_output;
    expected_ends.data = expected_output + 5;
    TEST((const char *)input, test_interpret_matches, input,
            IE_SUCCESS, expected_output,
            &expected_starts, &expected_ends, &starts, &ends);

    /* One match at the end */
    input = BAD_CAST "before" START_MARKER "MATCH" END_MARKER;
    expected_output = BAD_CAST "before" "MATCH";
    expected_starts.data = expected_output + 6;
    expected_ends.data = expected_output + 6+ 5;
    TEST((const char *)input, test_interpret_matches, input,
            IE_SUCCESS, expected_output,
            &expected_starts, &expected_ends, &starts, &ends);

    /* One empty match in the middle */
    input = BAD_CAST "before" START_MARKER END_MARKER "after";
    expected_output = BAD_CAST "before" "after";
    expected_starts.data = expected_output + 6;
    expected_ends.data = expected_output + 6;
    TEST((const char *)input, test_interpret_matches, input,
            IE_SUCCESS, expected_output,
            &expected_starts, &expected_ends, &starts, &ends);

    /* One non-empty match in the middle */
    input = BAD_CAST "before" START_MARKER "MATCH" END_MARKER "after";
    expected_output = BAD_CAST "before" "MATCH" "after";
    expected_starts.data = expected_output + 6;
    expected_ends.data = expected_output + 6 + 5;
    TEST((const char *)input, test_interpret_matches, input,
            IE_SUCCESS, expected_output,
            &expected_starts, &expected_ends, &starts, &ends);

    /* Only a start marker. This is ill but still acceptable by the
     * specification. */
    input = BAD_CAST "before" START_MARKER "MATCH";
    expected_output = BAD_CAST "before" "MATCH";
    expected_starts.data = expected_output + 6;
    TEST((const char *)input, test_interpret_matches, input,
            IE_SUCCESS, expected_output,
            &expected_starts, NULL, &starts, &ends);

    /* Only an end marker. This is ill but still acceptable by the
     * specification. */
    input = BAD_CAST "MATCH" END_MARKER "after";
    expected_output = BAD_CAST "MATCH" "after";
    expected_ends.data = expected_output + 5;
    TEST((const char *)input, test_interpret_matches, input,
            IE_SUCCESS, expected_output,
            NULL, &expected_ends, &starts, &ends);

    /* Two matches in the middle */
    input = BAD_CAST "before" START_MARKER "AMATCH1" END_MARKER "mid"
        START_MARKER "AMATCH02" END_MARKER "after";
    expected_output = BAD_CAST "before" "AMATCH1" "mid" "AMATCH02" "after";
    expected_starts.data = expected_output + 6;
    expected_starts.next = &expected_starts2;
    expected_ends.data = expected_output + 6 + 7;
    expected_ends.next = &expected_ends2;
    expected_starts2.data = expected_output + 6 + 7 + 3;
    expected_ends2.data = expected_output + 6 + 7 + 3 + 8;
    TEST((const char *)input, test_interpret_matches, input,
            IE_SUCCESS, expected_output,
            &expected_starts, &expected_ends, &starts, &ends);

    isds_list_free(&starts);
    isds_list_free(&ends);
    isds_cleanup();
    SUM_TEST();
}
