#include "../test.h"
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>

#define GNUPG_DIR ".gnupg"

#ifdef _WIN32
#define mkdir(x,y) mkdir(x)
#endif

static int test_setup_gnupg(void) {
    char *home, *path = NULL;
    char tmpfile[] = "XXXXXX";
    int tmpfd;

    home = getenv("HOME");
    if (!home) 
        FAIL_TEST("Could not get HOME variable");

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

    if (chdir(path)) {
        FAILURE_REASON("Could not change CWD to `%s': %s", path,
                strerror(errno));
        free(path);
        return 1;
    }

#ifndef _WIN32
    if (-1 == (tmpfd = mkstemp(tmpfile))) {
        FAILURE_REASON("Directory `%s' is not writable: %s", path,
                strerror(errno));
        free(path);
        return 1;
    }

    close(tmpfd);
    unlink(tmpfile);
#endif

    free(path);
    PASS_TEST;
}

int main(int argc, char **argv) {

    INIT_TEST("prepare_environment");

    TEST("set up gnupg", test_setup_gnupg);

    SUM_TEST();
}
