
#include <unistd.h> /* sleep() */

#include "../test.h"
#include "libdatovka/isds.h"
#include "common.h"

/*
 * The test requires MEP credentials to be stored in ../../test_credentials_mep.
 * The file must contain the username on the first line and the communication
 * key on the second line. The key and line must be related to a working account
 * in the ISDS testing environment.
 */

static
int test_login_mep2(const enum isds_error error, struct isds_ctx *context,
    const char *url, const char *username, const char *code,
    struct isds_mep *mep)
{
	enum isds_error err = isds_login_mep2(context, url, username, code, mep);
	if (error != err) {
		FAIL_TEST("Wrong return code: expected=%s, returned=%s (%s)",
		    isds_strerror(error), isds_strerror(err),
		    isds_long_message(context));
	}

	isds_logout(context);
	PASS_TEST;
}

static
int test_login_mep2_02(const enum isds_error error1, const enum isds_error error2,
    struct isds_ctx *context, const char *url, const char *username,
    const char *code, struct isds_mep *mep)
{
	enum isds_error err = isds_login_mep2(context, url, username, code, mep);
	if ((error1 != err) && (error2 != err)) {
		FAIL_TEST(
		    "Wrong return code: must_differ=%s, must_differ=%s, returned=%s (%s)",
		    isds_strerror(error1), isds_strerror(error2),
		    isds_strerror(err), isds_long_message(context));
	}

	isds_logout(context);
	PASS_TEST;
}

/* Complete login procedure. */
static
int test_login_mep2_cycle(const enum isds_error error,
    struct isds_ctx *context, const char *url, const char *username,
    const char *code, struct isds_mep *mep)
{
	isds_set_timeout(context, 60000);

	enum isds_error err = isds_login_mep2(context, url, username, code, mep);
	while (err == IE_PARTIAL_SUCCESS) {
		sleep(1);
		err = isds_login_mep2(context, url, username, code, mep);
		if (NULL == mep->ext_res) {
			FAIL_TEST("Expected to receive extended resolution status information.");
		}
		if (NULL == mep->ext_res->description) {
			FAIL_TEST("Expected to receive status description within extended resolution status information.");
		}
	}
	if (error != err) {
		FAIL_TEST("Wrong return code: expected=%s, returned=%s (%s)",
		    isds_strerror(error), isds_strerror(err),
		    isds_long_message(context));
	}

	isds_mep_ext_resolution_free(&(mep->ext_res));

	isds_logout(context);
	PASS_TEST;
}

int main(void) {
	INIT_TEST("login_mep");

	struct isds_ctx *context = NULL;
	const char *url = isds_mep_testing_locator;

	if (IE_SUCCESS != isds_init()) {
		ABORT_UNIT("isds_init() failed\n");
	}

	context = isds_ctx_create();
	if (NULL == context) {
		ABORT_UNIT("isds_ctx_create() failed\n");
	}

	struct isds_mep mep = {
		.app_name = "libdatovka-test",
		.intermediate_uri = NULL,
		.resolution = MEP_RESOLUTION_SUCCESS,
		.ext_res = NULL
	};

	TEST("invalid context", test_login_mep2, IE_INVALID_CONTEXT, NULL,
	    url, username_mep(), code_mep(), &mep);
	TEST("NULL URL with invalid credentials", test_login_mep2, IE_NOT_LOGGED_IN,
	    context, NULL, username_mep(), code_mep(), &mep);
	TEST("NULL username", test_login_mep2, IE_INVAL, context, url, NULL,
	    code_mep(), &mep);
	TEST("NULL communication code", test_login_mep2, IE_INVAL, context, url,
	    username_mep(), NULL, &mep);
	TEST("NULL MEP context", test_login_mep2, IE_INVAL, context, url,
	    username_mep(), code_mep(), NULL);

	TEST("invalid URL", test_login_mep2, IE_NETWORK, context,
	    "invalid://", username_mep(), code_mep(), &mep);
	/*
	 * Direct connection fails on local resolution, connection trough proxy
	 * fails on HTTP code.
	 */
	TEST("unresolvable host name", test_login_mep2_02, IE_NETWORK, IE_HTTP,
	    context, "http://unresolvable.example.com/", username_mep(),
	    code_mep(), &mep);

	TEST("invalid credentials", test_login_mep2, IE_NOT_LOGGED_IN, context,
	    url, "7777777", "nbuusr1", &mep);

	TEST("valid MEP login", test_login_mep2_cycle, IE_SUCCESS, context, url,
	    username_mep(), code_mep(), &mep);

	isds_ctx_free(&context);
	isds_cleanup();

	SUM_TEST();
}
