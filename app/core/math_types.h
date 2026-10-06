#ifndef YOUNG_MATH_TYPES_H
#define YOUNG_MATH_TYPES_H
#include <stdbool.h>
#include <stdint.h>
#define YT_DIM 64
#define YT_CELLS 256
typedef enum { YT_OK, YT_MALFORMED, YT_LIMIT, YT_OVERFLOW, YT_UNSUPPORTED, YT_COMPLEXITY } MathStatus;
typedef enum { ROW_INSERTION, COLUMN_INSERTION } InsertionConvention;
typedef enum { COLUMN_MINUS_ROW, ROW_MINUS_COLUMN } ContentConvention;
const char *math_status(MathStatus status);
#endif
