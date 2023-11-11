#include "pattern.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

bool pattern_matches(const char* pattern, const char* text)
{
    const size_t text_len = strlen(text);
    const size_t pattern_len = strlen(pattern);

    size_t pattern_idx = 0;

    for (size_t i = 0; i < text_len; ++i) {
        if (pattern_idx < pattern_len) {
            if (pattern[pattern_idx] == text[i]) {
                ++pattern_idx;
            }
        }
    }

    return pattern_idx >= pattern_len;
}
