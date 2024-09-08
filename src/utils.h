#pragma once

#include <stdbool.h>
#include <stdint.h>

/**
 * Check if two strings are equals.
 *
 * \param string1 the first string to check.
 * \param string2 the second string to check.
 *
 * \returns true if the string are equals.
 */
bool streq(const char *string1, const char *string2);

/**
 * Get the number of milliseconds since the system was booted.
 *
 * \return the time in milliseconds.
 */
uint64_t get_time_ms(void);
