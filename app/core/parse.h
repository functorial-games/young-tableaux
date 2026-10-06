#ifndef YOUNG_PARSE_H
#define YOUNG_PARSE_H
#include "math_types.h"
/* Shared text grammar. Mathematical operations accept objects, not strings. */
void parse_spaces(const char **cursor);
MathStatus parse_integer_list(const char **cursor,int *values,int *count,bool row);
#endif
