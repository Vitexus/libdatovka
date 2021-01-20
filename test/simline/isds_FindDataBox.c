#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE   /* For getaddrinfo(3) */
#endif

#ifndef _BSD_SOURCE
#define _BSD_SOURCE   /* For NI_MAXHOST up to glibc-2.19 */
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE   /* For NI_MAXHOST since glibc-2.20 */
#endif

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600   /* For unsetenv(3) */
#endif

#include "../test.h"
#include "server.h"
#include "libdatovka/isds.h"

static const char *username = "Doug1as$";
static const char *password = "42aA#bc8";


static int test_login(const isds_error error, struct isds_ctx *context,
        const char *url, const char *username, const char *password,
        const struct isds_pki_credentials *pki_credentials,
        struct isds_otp *otp) {
    isds_error err;

    err = isds_login(context, url, username, password, pki_credentials, otp);
    if (error != err)
        FAIL_TEST("Wrong return code: expected=%s, returned=%s (%s)",
                isds_strerror(error), isds_strerror(err),
                isds_long_message(context));

    PASS_TEST;
}

static int compare_isds_PersonName(
        const struct isds_PersonName *expected_result,
        const struct isds_PersonName *result) {

    TEST_POINTER_DUPLICITY(expected_result, result);
    if (NULL == expected_result)
        return 0;

    TEST_STRING_DUPLICITY(expected_result->pnFirstName, result->pnFirstName);
    TEST_STRING_DUPLICITY(expected_result->pnMiddleName, result->pnMiddleName);
    TEST_STRING_DUPLICITY(expected_result->pnLastName, result->pnLastName);
    TEST_STRING_DUPLICITY(expected_result->pnLastNameAtBirth,
            result->pnLastNameAtBirth);

    return 0;
};

static int compare_isds_BirthInfo(
        const struct isds_BirthInfo *expected_result,
        const struct isds_BirthInfo *result) {

    TEST_POINTER_DUPLICITY(expected_result, result);
    if (NULL == expected_result)
        return 0;

    TEST_TMPTR_DUPLICITY(expected_result->biDate, result->biDate);
    TEST_STRING_DUPLICITY(expected_result->biCity, result->biCity);
    TEST_STRING_DUPLICITY(expected_result->biCounty, result->biCounty);
    TEST_STRING_DUPLICITY(expected_result->biState, result->biState);

    return 0;
};

static int compare_isds_Address(
        const struct isds_Address *expected_result,
        const struct isds_Address *result) {

    TEST_POINTER_DUPLICITY(expected_result, result);
    if (NULL == expected_result)
        return 0;

    TEST_STRING_DUPLICITY(expected_result->adCity, result->adCity);
    TEST_STRING_DUPLICITY(expected_result->adStreet, result->adStreet);
    TEST_STRING_DUPLICITY(expected_result->adNumberInStreet,
            result->adNumberInStreet);
    TEST_STRING_DUPLICITY(expected_result->adNumberInMunicipality,
            result->adNumberInMunicipality);
    TEST_STRING_DUPLICITY(expected_result->adZipCode, result->adZipCode);
    TEST_STRING_DUPLICITY(expected_result->adState, result->adState);

    return 0;
};

static int compare_isds_DbOwnerInfo(
        const struct isds_DbOwnerInfo *expected_result,
        const struct isds_DbOwnerInfo *result) {

    TEST_POINTER_DUPLICITY(expected_result, result);
    if (NULL == expected_result)
        return 0;

    TEST_STRING_DUPLICITY(expected_result->dbID, result->dbID);
    TEST_INTPTR_DUPLICITY(expected_result->dbType, result->dbType);
    TEST_STRING_DUPLICITY(expected_result->ic, result->ic);
    if (compare_isds_PersonName(expected_result->personName,
                result->personName))
        return 1;
    TEST_STRING_DUPLICITY(expected_result->firmName, result->firmName);
    if (compare_isds_BirthInfo(expected_result->birthInfo, result->birthInfo))
        return 1;
    if (compare_isds_Address(expected_result->address, result->address))
        return 1;
    TEST_STRING_DUPLICITY(expected_result->nationality, result->nationality);
    TEST_STRING_DUPLICITY(expected_result->email, result->email);
    TEST_STRING_DUPLICITY(expected_result->telNumber, result->telNumber);
    TEST_STRING_DUPLICITY(expected_result->identifier, result->identifier);
    TEST_STRING_DUPLICITY(expected_result->registryCode, result->registryCode);
    TEST_INTPTR_DUPLICITY(expected_result->dbState, result->dbState);
    TEST_BOOLEANPTR_DUPLICITY(expected_result->dbEffectiveOVM,
            result->dbEffectiveOVM);
    TEST_BOOLEANPTR_DUPLICITY(expected_result->dbOpenAddressing,
            result->dbOpenAddressing);

    return 0;
}

static int compare_result_lists(const struct isds_list *expected_list,
        const struct isds_list *list) {
    const struct isds_list *expected_item, *item;
    int i;

    for (i = 1, expected_item = expected_list, item = list;
            NULL != expected_item && NULL != item;
            i++, expected_item = expected_item->next, item = item->next) {
        if (compare_isds_DbOwnerInfo(
                    (struct isds_DbOwnerInfo *)expected_item->data,
                    (struct isds_DbOwnerInfo *)item->data))
            return 1;
    }
    if (NULL != expected_item && NULL == item)
        FAIL_TEST("Result list is missing %d. item", i);
    if (NULL == expected_item && NULL != item)
        FAIL_TEST("Result list has superfluous %d. item", i);

    return 0;
}

static int test_isds_FindDataBox(const isds_error expected_error,
        struct isds_ctx *context,
        const struct isds_DbOwnerInfo *criteria,
        const struct isds_list *expected_results) {
    isds_error error;
    struct isds_list *results;
    TEST_CALLOC(results);

    error = isds_FindDataBox(context, criteria, &results);
    TEST_DESTRUCTOR((void(*)(void*))isds_list_free, &results);

    if (expected_error != error) {
        FAIL_TEST("Wrong return code: expected=%s, returned=%s (%s)",
                isds_strerror(expected_error), isds_strerror(error),
                isds_long_message(context));
    }

    if (IE_SUCCESS != error && IE_TOO_BIG != error) {
        TEST_POINTER_IS_NULL(results);
        PASS_TEST;
    }

    if (compare_result_lists(expected_results, results))
        return 1;

    
    PASS_TEST;
}

int main(void) {
    int error;
    pid_t server_process;
    struct isds_ctx *context = NULL;

    INIT_TEST("isds_FindDataBox");

    if (unsetenv("http_proxy")) {
        ABORT_UNIT("Could not remove http_proxy variable from environment\n");
    }
    if (isds_init()) {
        isds_cleanup();
        ABORT_UNIT("isds_init() failed\n");
    }
    context = isds_ctx_create();
    if (!context) {
        isds_cleanup();
        ABORT_UNIT("isds_ctx_create() failed\n");
    }

    {
        /* Full response with two results */
        char *url = NULL;

        struct isds_PersonName criteria_person_name = {
            .pnFirstName = "CN1",
            .pnMiddleName = "CN2",
            .pnLastName = "CN3",
            .pnLastNameAtBirth = "CN4"
        };
        struct tm criteria_biDate = {
            .tm_year = 4,
            .tm_mon = 5,
            .tm_mday = 6
        };
        struct isds_BirthInfo criteria_birth_info = {
            .biDate = &criteria_biDate,
            .biCity = "CB1",
            .biCounty = "CB2",
            .biState = "CB3"
        };
        struct isds_Address criteria_address = {
            .adCity = "CA1",
            .adStreet = "CA3",
            .adNumberInStreet = "CA4",
            .adNumberInMunicipality = "CA5",
            .adZipCode = "CA6",
            .adState = "CA7"
        };
        isds_DbType criteria_dbType = DBTYPE_OVM;
        long int criteria_dbState = 2;
        _Bool criteria_dbEffectiveOVM = 1;
        _Bool criteria_dbOpenAddressing = 1;
        struct isds_DbOwnerInfo criteria = {
            .dbID = "Cfoo123",
            .dbType = &criteria_dbType,
            .ic = "C1",
            .personName = &criteria_person_name,
            .firmName = "C2",
            .birthInfo = &criteria_birth_info,
            .address = &criteria_address,
            .nationality = "C3",
            .email = "C4",
            .telNumber = "C5",
            .identifier = "C6",
            .registryCode = "C7",
            .dbState = &criteria_dbState,
            .dbEffectiveOVM = &criteria_dbEffectiveOVM,
            .dbOpenAddressing = &criteria_dbOpenAddressing
        };
        struct server_owner_info server_criteria = {
            .dbID = criteria.dbID,
            .dbType = "OVM",
            .ic = criteria.ic,
            .pnFirstName = criteria.personName->pnFirstName,
            .pnMiddleName = criteria.personName->pnMiddleName,
            .pnLastName = criteria.personName->pnLastName,
            .pnLastNameAtBirth = criteria.personName->pnLastNameAtBirth,
            .firmName = criteria.firmName,
            .biDate = criteria.birthInfo->biDate,
            .biCity = criteria.birthInfo->biCity,
            .biCounty = criteria.birthInfo->biCounty,
            .biState = criteria.birthInfo->biState,
            .adCode = NULL,
            .adCity = criteria.address->adCity,
            .adDistrict = NULL,
            .adStreet = criteria.address->adStreet,
            .adNumberInStreet = criteria.address->adNumberInStreet,
            .adNumberInMunicipality = criteria.address->adNumberInMunicipality,
            .adZipCode = criteria.address->adZipCode,
            .adState = criteria.address->adState,
            .nationality = criteria.nationality,
            .email = criteria.email,
            .telNumber = criteria.telNumber,
            .identifier = criteria.identifier,
            .registryCode = criteria.registryCode,
            .dbState = criteria.dbState,
            .dbEffectiveOVM = criteria.dbEffectiveOVM,
            .dbOpenAddressing = criteria.dbOpenAddressing
        };

        struct isds_PersonName person_name = {
            .pnFirstName = "N1",
            .pnMiddleName = "N2",
            .pnLastName = "N3",
            .pnLastNameAtBirth = "N4"
        };
        struct tm biDate = {
            .tm_year = 1,
            .tm_mon = 2,
            .tm_mday = 3
        };
        struct isds_BirthInfo birth_info = {
            .biDate = &biDate,
            .biCity = "B1",
            .biCounty = "B2",
            .biState = "B3"
        };
        struct isds_Address address = {
            .adCity = "A1",
            .adStreet = "A3",
            .adNumberInStreet = "A4",
            .adNumberInMunicipality = "A5",
            .adZipCode = "A6",
            .adState = "A7"
        };
        isds_DbType dbType = DBTYPE_OVM;
        long int dbState = 2;
        _Bool dbEffectiveOVM = 1;
        _Bool dbOpenAddressing = 1;
        struct isds_DbOwnerInfo result = {
            .dbID = "foo1234",
            .dbType = &dbType,
            .ic = "1",
            .personName = &person_name,
            .firmName = "2",
            .birthInfo = &birth_info,
            .address = &address,
            .nationality = "3",
            .email = "4",
            .telNumber = "5",
            .identifier = "6",
            .registryCode = "7",
            .dbState = &dbState,
            .dbEffectiveOVM = &dbEffectiveOVM,
            .dbOpenAddressing = &dbOpenAddressing
        };
        struct isds_DbOwnerInfo result2 = {
            0
        };
        struct server_owner_info server_result = {
            .dbID = result.dbID,
            .dbType = "OVM",
            .ic = result.ic,
            .pnFirstName = result.personName->pnFirstName,
            .pnMiddleName = result.personName->pnMiddleName,
            .pnLastName = result.personName->pnLastName,
            .pnLastNameAtBirth = result.personName->pnLastNameAtBirth,
            .firmName = result.firmName,
            .biDate = result.birthInfo->biDate,
            .biCity = result.birthInfo->biCity,
            .biCounty = result.birthInfo->biCounty,
            .biState = result.birthInfo->biState,
            .adCode = NULL,
            .adCity = result.address->adCity,
            .adDistrict = NULL,
            .adStreet = result.address->adStreet,
            .adNumberInStreet = result.address->adNumberInStreet,
            .adNumberInMunicipality = result.address->adNumberInMunicipality,
            .adZipCode = result.address->adZipCode,
            .adState = result.address->adState,
            .nationality = result.nationality,
            .email_exists = 1,
            .email = result.email,
            .telNumber_exists = 1,
            .telNumber = result.telNumber,
            .identifier = result.identifier,
            .registryCode = result.registryCode,
            .dbState = result.dbState,
            .dbEffectiveOVM = result.dbEffectiveOVM,
            .dbOpenAddressing = result.dbOpenAddressing
        };
        struct server_owner_info server_result2 = {
            0
        };
        struct isds_list results2 = {
            .next = NULL,
            .data = &result2,
            .destructor = NULL
        };
        struct isds_list results = {
            .next = &results2,
            .data = &result,
            .destructor = NULL
        };
        struct server_list server_results2 = {
            .next = NULL,
            .data = &server_result2,
            .destructor = NULL
        };
        struct server_list server_results = {
            .next = &server_results2,
            .data = &server_result,
            .destructor = NULL
        };

        const struct arguments_DS_df_FindDataBox service_arguments = {
            .status_code = "0000",
            .status_message = "Ok.",
            .criteria = &server_criteria,
            .results_exists = 0,
            .results = &server_results
        };
        const struct service_configuration services[] = {
            { SERVICE_DS_Dz_DummyOperation, NULL },
            { SERVICE_DS_df_FindDataBox, &service_arguments },
            { SERVICE_END, NULL }
        };
        const struct arguments_basic_authentication server_arguments = {
            .username = username,
            .password = password,
            .isds_deviations = 1,
            .services = services
        };
        error = start_server(&server_process, &url,
                server_basic_authentication, &server_arguments, NULL);
        if (error == -1) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT(server_error);
        }
        TEST("login", test_login, IE_SUCCESS,
                context, url, username, password, NULL, NULL);
        free(url);

        TEST("All data", test_isds_FindDataBox, IE_SUCCESS,
                context, &criteria, &results);

        isds_logout(context);
        if (stop_server(server_process)) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT(server_error);
        }
    }

    {
        /* Truncated response with one result */
        char *url = NULL;

        struct isds_DbOwnerInfo criteria = {
            .dbID = "Cfoo123"
        };
        struct server_owner_info server_criteria = {
            .dbID = criteria.dbID
        };

        struct isds_PersonName person_name = {
            .pnFirstName = "N1",
            .pnMiddleName = "N2",
            .pnLastName = "N3",
            .pnLastNameAtBirth = "N4"
        };
        struct tm biDate = {
            .tm_year = 1,
            .tm_mon = 2,
            .tm_mday = 3
        };
        struct isds_BirthInfo birth_info = {
            .biDate = &biDate,
            .biCity = "B1",
            .biCounty = "B2",
            .biState = "B3"
        };
        struct isds_Address address = {
            .adCity = "A1",
            .adStreet = "A3",
            .adNumberInStreet = "A4",
            .adNumberInMunicipality = "A5",
            .adZipCode = "A6",
            .adState = "A7"
        };
        isds_DbType dbType = DBTYPE_OVM;
        long int dbState = 2;
        _Bool dbEffectiveOVM = 1;
        _Bool dbOpenAddressing = 1;
        struct isds_DbOwnerInfo result = {
            .dbID = "foo1234",
            .dbType = &dbType,
            .ic = "1",
            .personName = &person_name,
            .firmName = "2",
            .birthInfo = &birth_info,
            .address = &address,
            .nationality = "3",
            .email = "4",
            .telNumber = "5",
            .identifier = "6",
            .registryCode = "7",
            .dbState = &dbState,
            .dbEffectiveOVM = &dbEffectiveOVM,
            .dbOpenAddressing = &dbOpenAddressing
        };
        struct server_owner_info server_result = {
            .dbID = result.dbID,
            .dbType = "OVM",
            .ic = result.ic,
            .pnFirstName = result.personName->pnFirstName,
            .pnMiddleName = result.personName->pnMiddleName,
            .pnLastName = result.personName->pnLastName,
            .pnLastNameAtBirth = result.personName->pnLastNameAtBirth,
            .firmName = result.firmName,
            .biDate = result.birthInfo->biDate,
            .biCity = result.birthInfo->biCity,
            .biCounty = result.birthInfo->biCounty,
            .biState = result.birthInfo->biState,
            .adCode = NULL,
            .adCity = result.address->adCity,
            .adDistrict = NULL,
            .adStreet = result.address->adStreet,
            .adNumberInStreet = result.address->adNumberInStreet,
            .adNumberInMunicipality = result.address->adNumberInMunicipality,
            .adZipCode = result.address->adZipCode,
            .adState = result.address->adState,
            .nationality = result.nationality,
            .email_exists = 1,
            .email = result.email,
            .telNumber_exists = 1,
            .telNumber = result.telNumber,
            .identifier = result.identifier,
            .registryCode = result.registryCode,
            .dbState = result.dbState,
            .dbEffectiveOVM = result.dbEffectiveOVM,
            .dbOpenAddressing = result.dbOpenAddressing
        };
        struct isds_list results = {
            .next = NULL,
            .data = &result,
            .destructor = NULL
        };
        struct server_list server_results = {
            .next = NULL,
            .data = &server_result,
            .destructor = NULL
        };

        const struct arguments_DS_df_FindDataBox service_arguments = {
            .status_code = "0003",
            .status_message = "Answer was truncated",
            .criteria = &server_criteria,
            .results_exists = 0,
            .results = &server_results
        };
        const struct service_configuration services[] = {
            { SERVICE_DS_Dz_DummyOperation, NULL },
            { SERVICE_DS_df_FindDataBox, &service_arguments },
            { SERVICE_END, NULL }
        };
        const struct arguments_basic_authentication server_arguments = {
            .username = username,
            .password = password,
            .isds_deviations = 1,
            .services = services
        };
        error = start_server(&server_process, &url,
                server_basic_authentication, &server_arguments, NULL);
        if (error == -1) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT(server_error);
        }
        TEST("login", test_login, IE_SUCCESS,
                context, url, username, password, NULL, NULL);
        free(url);

        TEST("Truncated answer", test_isds_FindDataBox, IE_TOO_BIG,
                context, &criteria, &results);

        isds_logout(context);
        if (stop_server(server_process)) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT(server_error);
        }
    }



    {
        /* Client must refuse DBTYPE_OVM_MAIN and DBTYPE_SYSTEM */
        char *url = NULL;

        isds_DbType criteria_dbType = DBTYPE_OVM_MAIN;
        struct isds_DbOwnerInfo criteria = {
            .dbType = &criteria_dbType
        };

        const struct service_configuration services[] = {
            { SERVICE_DS_Dz_DummyOperation, NULL },
            { SERVICE_END, NULL }
        };
        const struct arguments_basic_authentication server_arguments = {
            .username = username,
            .password = password,
            .isds_deviations = 1,
            .services = services
        };
        error = start_server(&server_process, &url,
                server_basic_authentication, &server_arguments, NULL);
        if (error == -1) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT(server_error);
        }
        TEST("login", test_login, IE_SUCCESS,
                context, url, username, password, NULL, NULL);
        free(url);

        TEST("Invalid DBTYPE_OVM_MAIN", test_isds_FindDataBox, IE_ENUM,
                context, &criteria, NULL);

        criteria_dbType = DBTYPE_SYSTEM;
        TEST("Invalid DBTYPE_SYSTEM", test_isds_FindDataBox, IE_ENUM,
                context, &criteria, NULL);

        isds_logout(context);
        if (stop_server(server_process)) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT(server_error);
        }
    }

    {
        /* Report 0002 server error as IE_NONEXIST */
        char *url = NULL;

        struct isds_DbOwnerInfo criteria = {
            .dbID = "Cfoo123"
        };
        struct server_owner_info server_criteria = {
            .dbID = criteria.dbID
        };

        const struct arguments_DS_df_FindDataBox service_arguments = {
            .status_code = "0002",
            .status_message = "No such box",
            .criteria = &server_criteria,
            .results_exists = 0,
            .results = NULL
        };
        const struct service_configuration services[] = {
            { SERVICE_DS_Dz_DummyOperation, NULL },
            { SERVICE_DS_df_FindDataBox, &service_arguments },
            { SERVICE_END, NULL }
        };
        const struct arguments_basic_authentication server_arguments = {
            .username = username,
            .password = password,
            .isds_deviations = 1,
            .services = services
        };
        error = start_server(&server_process, &url,
                server_basic_authentication, &server_arguments, NULL);
        if (error == -1) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT(server_error);
        }
        TEST("login", test_login, IE_SUCCESS,
                context, url, username, password, NULL, NULL);
        free(url);

        TEST("Report 0002 server error as IE_NONEXIST", test_isds_FindDataBox,
                IE_NONEXIST, context, &criteria, NULL);

        isds_logout(context);
        if (stop_server(server_process)) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT(server_error);
        }
    }

    {
        /* Report 5001 server error as IE_NONEXIST */
        char *url = NULL;

        struct isds_DbOwnerInfo criteria = {
            .dbID = "Cfoo123"
        };
        struct server_owner_info server_criteria = {
            .dbID = criteria.dbID
        };

        const struct arguments_DS_df_FindDataBox service_arguments = {
            .status_code = "5001",
            .status_message = "No such box",
            .criteria = &server_criteria,
            .results_exists = 0,
            .results = NULL
        };
        const struct service_configuration services[] = {
            { SERVICE_DS_Dz_DummyOperation, NULL },
            { SERVICE_DS_df_FindDataBox, &service_arguments },
            { SERVICE_END, NULL }
        };
        const struct arguments_basic_authentication server_arguments = {
            .username = username,
            .password = password,
            .isds_deviations = 1,
            .services = services
        };
        error = start_server(&server_process, &url,
                server_basic_authentication, &server_arguments, NULL);
        if (error == -1) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT(server_error);
        }
        TEST("login", test_login, IE_SUCCESS,
                context, url, username, password, NULL, NULL);
        free(url);

        TEST("Report 0002 server error as IE_NONEXIST", test_isds_FindDataBox,
                IE_NONEXIST, context, &criteria, NULL);

        isds_logout(context);
        if (stop_server(server_process)) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT(server_error);
        }
    }




    isds_ctx_free(&context);
    isds_cleanup();
    SUM_TEST();
}
