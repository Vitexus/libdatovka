#include "test.h"
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#define GNUPG_DIR ".gnupg"

static int test_setup_gnupg(void) {
    char *home, *path = NULL;

    home = getenv("HOME");
    if (!home) 
        FAIL_TEST("Could not get home directory");

    if (!*home)
        FAIL_TEST("HOME environment variable is empty");

    if (0 > test_asprintf(&path, "%s/%s", home, GNUPG_DIR))
        FAIL_TEST("Could not build $HOME/.gnupg string");

    if (mkdir(path, S_IRWXU) && errno != EEXIST) {
        FAILURE_REASON("Could not create `%s' directory: %s", path,
                strerror(errno));
        free(path);
        return 1;
    }

    free(path);
    PASS_TEST;
}

int main(int argc, char **argv) {

    INIT_TEST("prepare_environment");

    TEST("set up gnupg", test_setup_gnupg);

    SUM_TEST();
}
