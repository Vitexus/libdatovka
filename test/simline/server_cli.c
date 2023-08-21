#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE   /* For getopt(3) */
#endif

#include "server.h"
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/types.h> /* For pid_t */
#include <string.h> /* memset() */
#include <unistd.h> /* getopt(3) */

static const char *username = NULL;
static const char *password = NULL;
static const char *otp_code = NULL;
static _Bool terminate = 0;
static int otp_type = 'n';


/* Signal handler */
static void terminator(int signal) {
    (void)signal;
    terminate = 1;
}

static void usage(const char *name) {
    printf("Usage: %s OPTIONS\n", name);
    printf(
            "\t-h HOTP_CODE     Define HMAC-based OTP code\n"
            "\t-p PASSWORD      Define password\n"
            "\t-t TOTP_CODE     Define time-based OTP code\n"
            "\t-u USERNAME      Define user name\n"
            "\t-a CERTIFICATE   PEM-formatted authority certificate\n"
            "\t-s CERTIFICATE   PEM-formatted server certificate\n"
            "\t-S KEY           PEM-formatted server private key\n"
            "\t-c NAME          Client distinguished name\n"
            );
}

int main(int argc, char **argv) {
    int error;
    pid_t server_process;
    char *server_address = NULL;
    int option;

    struct arguments_asws_changePassword_ChangePasswordOTP
        service_passwdotp_arguments;
    const struct arguments_asws_changePassword_SendSMSCode
            service_sendsms_arguments = {
        .status_code = "0000",
        .status_message = "OTP code sent",
        .reference_number = "43"
    };
    struct arguments_DS_DsManage_ChangeISDSPassword service_passwdbase_arguments;
    const struct arguments_DS_Dx_EraseMessage
            service_erasemessage_arguments = {
        .message_id = "1234567",
        .incoming = 1
    };
    struct tm date;
    const struct arguments_DS_Dz_ResignISDSDocument
            service_resigndocument_arguments = {
        .status_code = "0000",
        .status_message = "Document re-signed successfully",
        .valid_to = &date
    };
    struct service_configuration services[] = {
        { SERVICE_DS_Dx_EraseMessage, &service_erasemessage_arguments },
        { SERVICE_DS_Dz_DummyOperation, NULL },
        { SERVICE_DS_Dz_ResignISDSDocument, &service_resigndocument_arguments },
        { SERVICE_END, NULL }, /* This entry could be replaced later */
        { SERVICE_END, NULL }, /* This entry could be replaced later */
        { SERVICE_END, NULL }
    };
    int last_service = sizeof(services)/sizeof(services[0]) - 1;
    struct tls_authentication tls_arguments = {
        .authority_certificate = NULL,
        .server_certificate = NULL,
        .server_key = NULL,
        .client_name = NULL
    };
    struct arguments_basic_authentication server_basic_arguments;
    struct arguments_otp_authentication server_otp_arguments;

    memset(&date, 0, sizeof(date));
    date.tm_year = 42;
    date.tm_mon = 1;
    date.tm_mday = 1;

    /* Parse arguments */
    while (-1 != (option = getopt(argc, argv, "h:p:t:u:a:s:S:c:"))) {
        switch (option) {
            case 'h':
                otp_type = 'h';
                otp_code = optarg;
                break;
            case 'p':
                password = optarg;
                break;
            case 't':
                otp_type = 't';
                otp_code = optarg;
                break;
            case 'u':
                username = optarg;
                break;
            case 'a':
                tls_arguments.authority_certificate = optarg;
                break;
            case 's':
                tls_arguments.server_certificate = optarg;
                break;
            case 'S':
                tls_arguments.server_key = optarg;
                break;
            case 'c':
                tls_arguments.client_name = optarg;
                break;
            default:
                usage((argv != NULL) ? argv[0] : NULL);
                exit(EXIT_FAILURE);

        }
    }
    if (optind != argc) {
        fprintf(stderr, "Superfluous argument\n");
        usage((argv != NULL) ? argv[0] : NULL);
        exit(EXIT_FAILURE);
    }

    /* Configure server */
    if (otp_type == 'n') {
        service_passwdbase_arguments.username = username;
        service_passwdbase_arguments.current_password = password;
        services[last_service-2].name = SERVICE_DS_DsManage_ChangeISDSPassword;
        services[last_service-2].arguments = &service_passwdbase_arguments;
        server_basic_arguments.username = username;
        server_basic_arguments.password = password;
        server_basic_arguments.isds_deviations = 1;
        server_basic_arguments.services = services;
    } else {
        service_passwdotp_arguments.username = username;
        service_passwdotp_arguments.current_password = password;
        service_passwdotp_arguments.reference_number = "42";
        services[last_service-2].name =
            SERVICE_asws_changePassword_ChangePasswordOTP;
        services[last_service-2].arguments = &service_passwdotp_arguments;
        services[last_service-1].name =
            SERVICE_asws_changePassword_SendSMSCode;
        services[last_service-1].arguments = &service_sendsms_arguments;
        server_otp_arguments.otp = otp_code;
        if (otp_type == 't') {
            server_otp_arguments.method = AUTH_OTP_TIME;
        } else if (otp_type == 'h') {
            server_otp_arguments.method = AUTH_OTP_HMAC;
        } else {
            fprintf(stderr, "Internal error: Unknown OTP type: %c\n", otp_type);
            exit(EXIT_FAILURE);
        }
        service_passwdotp_arguments.method = server_otp_arguments.method;
        server_otp_arguments.username = username;
        server_otp_arguments.password = password;
        server_otp_arguments.isds_deviations = 1;
        server_otp_arguments.services = services;
    }

    /* Spawn the server */
    if ((SIG_ERR == signal(SIGTERM, terminator))) {
        fprintf(stderr, "Could not register SIGTERM handler\n");
        exit(EXIT_FAILURE);
    }
    if ((SIG_ERR == signal(SIGCHLD, terminator))) {
        fprintf(stderr, "Could not register SIGCHLD handler\n");
        exit(EXIT_FAILURE);
    }

    printf("Starting server on:\n");
    if (otp_type == 'n') {
        error = start_server(&server_process, &server_address,
                server_basic_authentication, &server_basic_arguments,
                &tls_arguments);
    } else {
        error = start_server(&server_process, &server_address,
                server_otp_authentication, &server_otp_arguments,
                &tls_arguments);
    }
    if (error == -1) {
        fprintf(stderr, "Could not start server: %s\n", server_error);
        free(server_error);
        free(server_address);
        exit(EXIT_FAILURE);
    }
    printf("%s\n", server_address);
    free(server_address);

    printf("Waiting on SIGTERM...\n");
    while (!terminate) {
        select(0, NULL, NULL, NULL, NULL);
    }

    printf("Terminating...\n");
    error = stop_server(server_process);
    if (-1 == error) {
        fprintf(stderr, "Could not stop server: %s\n", server_error);
        free(server_error);
        exit(EXIT_FAILURE);
    } else if (error) {
        fprintf(stderr, "Server crashed: %s\n", server_error);
        free(server_error);
        exit(EXIT_FAILURE);
    }

    printf("Terminated.\n");
    exit(EXIT_SUCCESS);
}
