#define _XOPEN_SOURCE 700
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char credentials_pwd_file[] = "../../test_credentials";
char credentials_mep_file[] = "../../test_credentials_mep";

static int read_config(const char *credentials_file, char **line, int order) {
    FILE *file;
    size_t length = 0;
    char *eol;

    if (NULL == credentials_file) {
        return -1;
    }

    if (!line) return -1;
    free(*line);
    *line = NULL;

    file = fopen(credentials_file, "r");
    if (!file) {
        fprintf(stderr, "Could not open %s\n", credentials_file);
        return -1;
    }

    for (int i = 0; i < order; i++) {
        if (-1 == getline(line, &length, file)) {
            fprintf(stderr, "Could not read line #%d from %s: ",
                    i + 1, credentials_file);
            if (ferror(file))
                fprintf(stderr, "error occured\n");
            else if (feof(file))
                fprintf(stderr, "end of file reached\n");
            else
                fprintf(stderr, "I don't know why\n");
            fclose(file);
            free(*line);
            *line = NULL;
            return -1;
        }
    }

    fclose(file);

    eol = strpbrk(*line, "\r\n");
    if (eol) *eol = '\0';

    return 0;
}

const char *username(void) {
    static char *username = NULL;

    if (NULL == username) {
        username = getenv("ISDS_USERNAME");
        if (NULL == username)
            read_config(credentials_pwd_file, &username, 1);
    }
    return username;
}

const char *password(void) {
    static char *password = NULL;

    if (NULL == password) {
        password = getenv("ISDS_PASSWORD");
        if (NULL == password)
            read_config(credentials_pwd_file, &password, 2);
    }
    return password;
}

const char *username_mep(void) {
    static char *username = NULL;

    if (NULL == username) {
        username = getenv("ISDS_USERNAME");
        if (NULL == username) {
            read_config(credentials_mep_file, &username, 1);
        }
    }
    return username;
}

const char *code_mep(void) {
    static char *code = NULL;

    if (NULL == code) {
        code = getenv("ISDS_CODE_MEP");
        if (NULL == code) {
            read_config(credentials_mep_file, &code, 2);
        }
    }
    return code;
}
