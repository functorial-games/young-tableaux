#include "parse.h"
#include <ctype.h>
#include <limits.h>
#include <string.h>

const char *math_status(MathStatus s)
{
    switch (s) {
    case YT_OK: return "OK";
    case YT_MALFORMED: return "INVALID INPUT";
    case YT_LIMIT: return "LIMIT: 64 rows/columns, 256 cells";
    case YT_OVERFLOW: return "OVERFLOW: exact uint64 range exceeded";
    case YT_UNSUPPORTED: return "NOT IMPLEMENTED";
    }
    return "ERROR";
}

/* Strict list grammar: optional [], whitespace/comma-separated signed decimals.
 * Semicolons belong only to the tableau row grammar. Never accept a prefix. */
void parse_spaces(const char **p) { while (isspace((unsigned char)**p)) ++*p; }
MathStatus parse_integer_list(const char **p, int *values, int *count, bool row)
{
    *count = 0;
    parse_spaces(p);
    bool bracket = **p == '[';
    if (bracket) { ++*p; parse_spaces(p); }
    if (**p == ']' && bracket) { ++*p; return YT_OK; }
    if (**p == '\0' && !bracket) return YT_OK;
    for (;;) {
        bool negative = **p == '-';
        if (negative || **p == '+') ++*p;
        if (!isdigit((unsigned char)**p)) return YT_MALFORMED;
        uint64_t value = 0;
        while (isdigit((unsigned char)**p)) {
            unsigned digit = (unsigned)(**p - '0');
            uint64_t bound = negative ? (uint64_t)INT_MAX + 1 : INT_MAX;
            if (value > (bound - digit) / 10) return YT_OVERFLOW;
            value = value * 10 + digit;
            ++*p;
        }
        if (*count == YT_DIM) return YT_LIMIT;
        values[(*count)++] = negative ? (value == (uint64_t)INT_MAX + 1 ? INT_MIN : -(int)value) : (int)value;
        bool had_space = isspace((unsigned char)**p) != 0;
        parse_spaces(p);
        if (bracket && **p == ']') { ++*p; return YT_OK; }
        if (!bracket && (**p == '\0' || (row && **p == ';'))) return YT_OK;
        if (**p == ',') { ++*p; parse_spaces(p); }
        else if (!had_space) return YT_MALFORMED;
        if (!isdigit((unsigned char)**p) && **p != '-' && **p != '+') return YT_MALFORMED;
    }
}
