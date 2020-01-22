#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef _WIN32
#include <sys/mman.h>
#endif

#include <errno.h>
#include <string.h>
#include <unistd.h>

/* Global variables for each test unit */
char *unit_name = NULL;
char *reason = NULL;
unsigned int passed, failed, skipped;
void (*test_destructor_function)(void *) = NULL;
void *test_destructor_argument = NULL;

/* Print formated string into automtically reallocated @uffer.
 * @buffer automatically reallocated buffer. Must be &NULL or preallocated
 * memory.
 * @format format string as for printf(3)
 * @ap list of variadic arguments, after call will be in udefined state
 * @Returns number of bytes printed. In case of errror, -1 and NULL @buffer */
int test_vasprintf(char **buffer, const char *format, va_list ap) {
    va_list aq;
    int length, new_length;
    char *new_buffer;

    if (!buffer || !format) {
        if (buffer) {
            free(*buffer);
            *buffer = NULL;
        }
        return -1;
    }

    va_copy(aq, ap);
    length = vsnprintf(NULL, 0, format, aq) + 1;
    va_end(aq);
    if (length <= 0) {
        free(*buffer);
        *buffer = NULL;
        return -1;
    }

    new_buffer = realloc(*buffer, length);
    if (!new_buffer) {
        free(*buffer);
        *buffer = NULL;
        return -1;
    }
    *buffer = new_buffer;

    new_length = vsnprintf(*buffer, length, format, ap);
    if (new_length >= length) {
        free(*buffer);
        *buffer = NULL;
        return -1;
    }

    return new_length;
}


/* Print formated string into automtically reallocated @uffer.
 * @buffer automatically reallocated buffer. Must be &NULL or preallocated
 * memory.
 * @format format string as for printf(3)
 * @... variadic arguments
 * @Returns number of bytes printed. In case of errror, -1 and NULL @buffer */
int test_asprintf(char **buffer, const char *format, ...) {
    int ret;
    va_list ap;
    va_start(ap, format);
    ret = test_vasprintf(buffer, format, ap);
    va_end(ap);
    return ret;
}


#ifdef _WIN32
int test_mmap_file(const char *file, int *fd, void **buffer, size_t *length) {
    struct stat file_info;
    int ret, pos = 0;

    if (!file || !fd || !buffer || !length) return -1;

    *fd = open(file, O_RDONLY);
    if (*fd == -1) {
        fprintf(stderr, "%s: Could not open file: %s\n", file, strerror(errno));
        return -1;
    }

    if (-1 == fstat(*fd, &file_info)) {
        fprintf(stderr, "%s: Could not get file size: %s\n", file,
                strerror(errno));
        close(*fd);
        return -1;
    }
    if (file_info.st_size < 0) {
        fprintf(stderr, "File `%s' has negative size: %" PRIdMAX "\n", file,
                (intmax_t) file_info.st_size);
        close(*fd);
        return -1;
    }
    *length = file_info.st_size;
    *buffer = malloc(*length);

    if (!*buffer) {
        fprintf(stderr, "%s: Could not allocate memory for file mapping: %s\n",
                file, strerror(errno));
        close(*fd);
        return -1;
    }

    do {
        ret = read(*fd, *buffer + pos, *length - pos);

        if (ret > 0) {
            pos += ret;
        }
    } while ((ret < 0 && errno == EINTR) || (ret > 0 && pos < *length));

    if (ret < 0) {
        fprintf(stderr, "%s: Could not map file to memory: %s\n", file,
                strerror(errno));
        free(*buffer);
        *buffer = NULL;
        close(*fd);
        return -1;
    }

    return 0;
}

int test_munmap_file(int fd, void *buffer, size_t length) {
    int err = 0;
    free(buffer);

    err = close(fd);
    if (err) {
        fprintf(stderr, "Could close file descriptor %d: %s\n", fd,
                strerror(errno));
    }

    return err;
}
#else
int test_mmap_file(const char *file, int *fd, void **buffer, size_t *length) {
    struct stat file_info;

    if (!file || !fd || !buffer || !length) return -1;


    *fd = open(file, O_RDONLY);
    if (*fd == -1) {
        fprintf(stderr, "%s: Could not open file: %s\n", file, strerror(errno));
        return -1;
    }

    if (-1 == fstat(*fd, &file_info)) {
        fprintf(stderr, "%s: Could not get file size: %s\n", file,
                strerror(errno));
        close(*fd);
        return -1;
    }
    if (file_info.st_size < 0) {
        fprintf(stderr, "File `%s' has negative size: %" PRIdMAX "\n", file,
                (intmax_t) file_info.st_size);
        close(*fd);
        return -1;
    }
    *length = file_info.st_size;

    *buffer = mmap(NULL, *length, PROT_READ, MAP_PRIVATE, *fd, 0);
    if (*buffer == MAP_FAILED) {
        fprintf(stderr, "%s: Could not map file to memory: %s\n", file,
                strerror(errno));
        close(*fd);
        return -1;
    }

    return 0;
}


int test_munmap_file(int fd, void *buffer, size_t length) {
    int err = 0;
    long int page_size = sysconf(_SC_PAGE_SIZE);
    size_t pages = (length % page_size) ?
        ((length / page_size) + 1) * page_size:
        length;

    err = munmap(buffer, pages);
    if (err) {
        fprintf(stderr, "Could not unmap memory at %p and length %zu: %s\n",
                buffer, pages, strerror(errno));
    }

    err = close(fd);
    if (err) {
        fprintf(stderr, "Could close file descriptor %d: %s\n", fd,
                strerror(errno));
    }

    return err;
}
#endif

