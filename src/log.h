#pragma once

#include <stdbool.h>
#include <stddef.h>

bool log_init(const char *program_name);
void log_quit(void);

#ifndef PROD
#define PRINTF_LIKE __attribute__((format(printf, 3, 4)))
#define file_and_line_param const char *file, const size_t line,

#define log_errorf(format, ...) _log_errorf(__FILE__, __LINE__, (format) \
                                            __VA_OPT__(,) __VA_ARGS__)

#define log_debugf(format, ...) _log_debugf(__FILE__, __LINE__, (format) \
                                            __VA_OPT__(,) __VA_ARGS__)
void _log_debugf(const char *file, const size_t line, const char *format, ...
                 ) PRINTF_LIKE;
#else
#define PRINTF_LIKE __attribute__((format(printf, 1, 2)))
#define file_and_line_param

#define log_errorf(format, ...) _log_errorf((format) __VA_OPT__(,) __VA_ARGS__)
#define log_debugf(format, ...)
#endif

void _log_errorf(file_and_line_param const char *format, ...) PRINTF_LIKE;
