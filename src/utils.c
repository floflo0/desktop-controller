#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "utils.h"

bool streq(const char *string1, const char *string2){
    return strcmp(string1, string2) == 0;
}

uint64_t get_time_ms(void) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    return now.tv_sec * 1000 + now.tv_nsec / 1000000;
}
