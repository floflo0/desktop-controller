#pragma once

/**
 * Logging system for the application.
 */

#include <stdbool.h>
#include <stddef.h>

/**
 * Initialize the logging system.
 *
 * \param program_name The name of the program being executed.
 *
 * \returns true on success, or false on failure.
 */
bool log_init(const char *program_name);

/**
 * Free the resources used by the logging system.
 */
void log_quit(void);

#ifndef PROD
#define PRINTF_LIKE __attribute__((format(printf, 3, 4)))
#define file_and_line_param const char *file, const size_t line,

#define log_errorf(format, ...) _log_errorf(__FILE__, __LINE__, (format) \
                                            __VA_OPT__(,) __VA_ARGS__)

#define log_debugf(format, ...) _log_debugf(__FILE__, __LINE__, (format) \
                                            __VA_OPT__(,) __VA_ARGS__)
/**
 * Log a debug message with the given format prefixed by the file and the line
 * from where the message was printed.
 *
 * Use the log_debugf() so the file and line parameters are filled automatically.
 * In production build, the debug call will be removed.
 *
 * \param file The path of the file from were the message is printed.
 * \param line The line number from were the message is printed.
 * \param format A printf like format for the log message.
 */
void _log_debugf(const char *file, const size_t line, const char *format, ...
                 ) PRINTF_LIKE;
#else
#define PRINTF_LIKE __attribute__((format(printf, 1, 2)))
#define file_and_line_param

#define log_errorf(format, ...) _log_errorf((format) __VA_OPT__(,) __VA_ARGS__)
#define log_debugf(format, ...)
#endif

/**
 * Log an error message with the given format.
 *
 * In non-production build, this function also take the file and the line from
 * where the message is printed for debug purpose.
 * Use the log_debugf() so the file and line parameters are filled
 * automatically and in production, the location of the call will be removed.
 *
 * \param file Only in debug! The path of the file from were the message is
 *             printed.
 * \param line Only in debug! The line number from were the message is printed.
 * \param format A printf like format for the log message.
 */
void _log_errorf(file_and_line_param const char *format, ...) PRINTF_LIKE;
