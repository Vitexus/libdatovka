#include "../test.h"
#include "isds.c"

static
int test_string2isds_credit_event_type(const xmlChar *string,
    enum isds_error error, enum isds_credit_event_type *type)
{
	enum isds_error err;
	enum isds_credit_event_type new_type;

	err = string2isds_credit_event_type(string, (type) ? &new_type : NULL);
	if (err != error) {
		FAIL_TEST(
		    "string2isds_credit_event_type() returned wrong exit code: expected=%s, got=%s",
		    isds_strerror(error), isds_strerror(err));
	}
	if (err != IE_SUCCESS) {
		PASS_TEST;
	}
	if (*type != new_type) {
		FAIL_TEST(
		    "string2isds_credit_event_type() returned wrong type: expected=%d, got=%d",
		    *type, new_type);
	}
	PASS_TEST;
}

int main(void)
{
	INIT_TEST("credit_event_type conversion");

	const xmlChar *names[] = {
		BAD_CAST "1",
		BAD_CAST "2",
		BAD_CAST "3",
		BAD_CAST "4",
		BAD_CAST "5",
		BAD_CAST "7"
	};

	isds_credit_event_type types[] =  {
		ISDS_CREDIT_CHARGED,
		ISDS_CREDIT_DISCHARGED,
		ISDS_CREDIT_MESSAGE_SENT,
		ISDS_CREDIT_STORAGE_SET,
		ISDS_CREDIT_EXPIRED,
		ISDS_CREDIT_DELETED_MESSAGE_RECOVERED
	};

	/* Known values. */
	for (size_t i = 0; i < sizeof(types)/sizeof(types[0]); ++i) {
		TEST(names[i], test_string2isds_credit_event_type,
		    names[i], IE_SUCCESS, &types[i]);
	}

	/* Unknown value. */
	TEST("X-Invalid_Type", test_string2isds_credit_event_type,
	    BAD_CAST "X-Invalid_Type", IE_ENUM, &types[0]);

	/* Invalid invocation. */
	TEST("NULL string", test_string2isds_credit_event_type,
	    BAD_CAST NULL, IE_INVAL, &types[0]);
	TEST("NULL type", test_string2isds_credit_event_type,
	    names[0], IE_INVAL, NULL);
	SUM_TEST();
}
