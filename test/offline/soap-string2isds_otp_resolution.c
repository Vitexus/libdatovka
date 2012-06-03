#include "../test.h"
#include "soap.c"

static int test_string2isds_otp_resolution(const char *string,
        const isds_otp_resolution correct_resolution) {
    isds_otp_resolution new_resolution;

    new_resolution = string2isds_otp_resolution(string);
    if (new_resolution != correct_resolution)
        FAIL_TEST("string2isds_otp_resolution() returned unexpected value: "
                "expected=%d got=%d", correct_resolution, new_resolution);
    PASS_TEST;
}

int main(int argc, char **argv) {
    int i;

    const char *hotp_strings[] = {
        "authentication.error.userIsNotAuthenticated",
        "authentication.error.intruderDetected",
        "authentication.error.paswordExpired",
        "authentication.error.badRole"
    };
    const isds_otp_resolution hotp_resolutions[] = {
        OTP_RESOLUTION_BAD_AUTHENTICATION,
        OTP_RESOLUTION_ACCESS_BLOCKED,
        OTP_RESOLUTION_PASSWORD_EXPIRED,
        OTP_RESOLUTION_UNAUTHORIZED
    };

    const char *totp_strings[] = {
		"authentication.info.totpSended",
		"authentication.error.userIsNotAuthenticated",
		"authentication.error.intruderDetected",
		"authentication.error.paswordExpired",
		"authentication.info.cannotSendQuickly",
		"authentication.error.badRole",
		"authentication.info.totpNotSended"
    };
    const isds_otp_resolution totp_resolutions[] = {
        OTP_RESOLUTION_TOTP_SENT,
        OTP_RESOLUTION_BAD_AUTHENTICATION,
        OTP_RESOLUTION_ACCESS_BLOCKED,
        OTP_RESOLUTION_PASSWORD_EXPIRED,
        OTP_RESOLUTION_TO_FAST,
        OTP_RESOLUTION_UNAUTHORIZED,
        OTP_RESOLUTION_TOTP_NOT_SENT
    };

    INIT_TEST("OTP X-Response-message-Code string to isds_otp_resolution "
            "conversion");

    /* HOTP */
    for (i = 0; i < sizeof(hotp_strings)/sizeof(hotp_strings[0]); i++) {
        TEST(hotp_strings[i], test_string2isds_otp_resolution,
                hotp_strings[i], hotp_resolutions[i]);
    }

    /* TOTP */
    for (i = 0; i < sizeof(totp_strings)/sizeof(totp_strings[0]); i++) {
        TEST(totp_strings[i], test_string2isds_otp_resolution,
                totp_strings[i], totp_resolutions[i]);
    }
    
    /* Corner cases */
    TEST("Unknown value", test_string2isds_otp_resolution, "X-Unknown value",
            OTP_RESOLUTION_UNKNOWN);
    TEST("Empty string", test_string2isds_otp_resolution, "",
            OTP_RESOLUTION_UNKNOWN);
    TEST("NULL pointer", test_string2isds_otp_resolution, NULL,
            OTP_RESOLUTION_UNKNOWN);

    SUM_TEST();
}
