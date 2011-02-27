#define _XOPEN_SOURCE 700
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char credentials_file[] = "../../test_credentials";

static int read_config(char **line, int order) {
    FILE *file;
    size_t length = 0;
    char *eol;

    if (!line) return -1;
    free(*line);
    *line = NULL;

    file = fopen(credentials_file, "r");
    if (!file) {
        fprintf(stderr, "Could open %s\n", credentials_file);
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
    static char *username;

    if (!username) {
        username = getenv("ISDS_USERNAME");
        if (!username)
            read_config(&username, 1);
    }
    return username;
}

const char *password(void) {
    static char *password;

    if (!password) {
        password = getenv("ISDS_PASSWORD");
        if (!password)
            read_config(&password, 2);
    }
    return password;
}

