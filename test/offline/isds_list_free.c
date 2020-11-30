#include "../test.h"
#include "libdatovka/isds.h"

int counter;

static void destructor(void **data) {
    (void)data;
    counter++;
}

static int test_isds_list_free(struct isds_list **list, int destructor_hits) {
    counter = 0;

    isds_list_free(list);

    if (destructor_hits != counter)
        FAIL_TEST("Destructor called wrong times: expected=%d, called=%d",
                destructor_hits, counter);
    
    if (!list) PASS_TEST;

    if (*list)
        FAIL_TEST("isds_list_free() did not null pointer");

    PASS_TEST;
}


int main(void) {

    INIT_TEST("isds_list_free()");
    if (isds_init())
        ABORT_UNIT("isds_init() failed");

    
    /* isds_list_free() */
    struct isds_list *list = NULL;
    TEST("NULL", test_isds_list_free, NULL, 0);
    TEST("*NULL", test_isds_list_free, &list, 0);

    TEST_CALLOC(list);
    TEST("One item without destructor", test_isds_list_free, &list, 0);

    TEST_CALLOC(list);
    list->destructor = destructor;
    TEST("One item with destructor", test_isds_list_free, &list, 1);

    TEST_CALLOC(list);
    list->destructor = destructor;
    TEST_CALLOC(list->next);
    list->next->destructor = destructor;
    TEST("Two items list", test_isds_list_free, &list, 2);

    isds_cleanup();
    SUM_TEST();
}
