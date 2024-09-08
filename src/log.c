#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"

static char *program_name = NULL;

bool log_init(const char *new_program_name) {
    const size_t program_name_size = strlen(new_program_name) + 1;
    program_name = malloc(program_name_size);
    if (!program_name) {
        fprintf(stderr, "%s: error: failed to allocate memory: %s\n",
                new_program_name, strerror(errno));
        return false;
    }
    memcpy(program_name, new_program_name, program_name_size);

    return true;
}

void log_quit(void) {
    free(program_name);
}

void _log_errorf(file_and_line_param const char *format, ...) {
    assert(program_name && "log module hasn't been initialized");
#ifndef PROD
    fprintf(stderr, "%s:%lu: ", file, line);
#endif
    fprintf(stderr, "%s: error: ", program_name);
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputc('\n', stderr);
}

#ifndef PROD
void _log_debugf(const char *file, const size_t line, const char *format, ...) {
    printf("%s:%lu: %s: debug: ", file, line, program_name);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    fputc('\n', stdout);
}
#endif
