#include "young_math.h"
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
    }
    return "ERROR";
}

/* Strict list grammar: optional [], whitespace/comma-separated signed decimals.
 * Semicolons belong only to the tableau row grammar. Never accept a prefix. */
static void spaces(const char **p) { while (isspace((unsigned char)**p)) ++*p; }
static MathStatus list(const char **p, int *values, int *count, bool row)
{
    *count = 0;
    spaces(p);
    bool bracket = **p == '[';
    if (bracket) { ++*p; spaces(p); }
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
        spaces(p);
        if (bracket && **p == ']') { ++*p; return YT_OK; }
        if (!bracket && (**p == '\0' || (row && **p == ';'))) return YT_OK;
        if (**p == ',') { ++*p; spaces(p); }
        else if (!had_space) return YT_MALFORMED;
        if (!isdigit((unsigned char)**p) && **p != '-' && **p != '+') return YT_MALFORMED;
    }
}
MathStatus partition_parse(const char *text, Partition *out)
{
    Partition tmp = {0};
    const char *p = text;
    MathStatus s = list(&p, tmp.rows, &tmp.count, false);
    if (s != YT_OK) return s;
    spaces(&p);
    if (*p) return YT_MALFORMED;
    int size = 0;
    for (int r = 0; r < tmp.count; ++r) {
        if (tmp.rows[r] <= 0 || (r && tmp.rows[r] > tmp.rows[r-1])) return YT_MALFORMED;
        if (tmp.rows[r] > YT_DIM || size > YT_CELLS - tmp.rows[r]) return YT_LIMIT;
        size += tmp.rows[r];
    }
    *out = tmp;
    return YT_OK;
}
int partition_size(const Partition *p)
{ int size = 0; for (int r = 0; r < p->count; ++r) size += p->rows[r]; return size; }
Partition partition_conjugate(const Partition *p)
{
    Partition out = {0};
    if (p->count) out.count = p->rows[0];
    for (int c = 0; c < out.count; ++c)
        for (int r = 0; r < p->count; ++r) out.rows[c] += p->rows[r] > c;
    return out;
}
int partition_cells(const Partition *p, Cell *out)
{
    int n = 0;
    for (int r = 0; r < p->count; ++r)
        for (int c = 0; c < p->rows[r]; ++c) out[n++] = (Cell){r+1,c+1};
    return n;
}
int partition_removable(const Partition *p, Cell *out)
{
    int n = 0;
    for (int r = 0; r < p->count; ++r)
        if (r+1 == p->count || p->rows[r] > p->rows[r+1]) out[n++] = (Cell){r+1,p->rows[r]};
    return n;
}
int partition_addable(const Partition *p, Cell *out)
{
    int n = 0;
    for (int r = 0; r < p->count; ++r)
        if (!r || p->rows[r-1] > p->rows[r]) out[n++] = (Cell){r+1,p->rows[r]+1};
    out[n++] = (Cell){p->count+1,1};
    return n;
}
int partition_hook(const Partition *p, int r, int c)
{
    if (r < 1 || r > p->count || c < 1 || c > p->rows[r-1]) return 0;
    int h = p->rows[r-1] - c + 1;
    for (int i = r; i < p->count; ++i) h += p->rows[i] >= c;
    return h;
}
MathStatus partition_hook_product(const Partition *p, uint64_t *out)
{
    uint64_t value = 1;
    for (int r = 1; r <= p->count; ++r) for (int c = 1; c <= p->rows[r-1]; ++c) {
        uint64_t h = (uint64_t)partition_hook(p,r,c);
        if (value > UINT64_MAX / h) return YT_OVERFLOW;
        value *= h;
    }
    *out = value; return YT_OK;
}
/* Prime cancellation computes n!/product(h) without overflowing n! or the
 * hook product when the final quotient still fits. */
MathStatus partition_standard_count(const Partition *p, uint64_t *out)
{
    int exponents[YT_CELLS+1] = {0};
    int n = partition_size(p);
    for (int k = 2; k <= n; ++k) {
        int v = k;
        for (int d = 2; d <= v; ++d) while (v % d == 0) { ++exponents[d]; v /= d; }
    }
    for (int r = 1; r <= p->count; ++r) for (int c = 1; c <= p->rows[r-1]; ++c) {
        int v = partition_hook(p,r,c);
        for (int d = 2; d <= v; ++d) while (v % d == 0) { --exponents[d]; v /= d; }
    }
    uint64_t value = 1;
    for (int d = 2; d <= n; ++d) {
        if (exponents[d] < 0) return YT_MALFORMED;
        for (int k = 0; k < exponents[d]; ++k) {
            if (value > UINT64_MAX / (unsigned)d) return YT_OVERFLOW;
            value *= (unsigned)d;
        }
    }
    *out = value; return YT_OK;
}
MathStatus tableau_parse(const char *text, Tableau *out)
{
    Tableau tmp = {0}; const char *p = text; int size = 0;
    spaces(&p);
    if (!*p || strcmp(p,"[]") == 0) { *out = tmp; return YT_OK; }
    for (;;) {
        int r = tmp.shape.count;
        if (r == YT_DIM) return YT_LIMIT;
        MathStatus s = list(&p,tmp.entries[r],&tmp.shape.rows[r],true);
        if (s != YT_OK) return s;
        if (!tmp.shape.rows[r]) {
            spaces(&p);
            if (!r && !*p) { *out = tmp; return YT_OK; }
            return YT_MALFORMED;
        }
        size += tmp.shape.rows[r];
        if (size > YT_CELLS) return YT_LIMIT;
        ++tmp.shape.count; spaces(&p);
        if (!*p) break;
        if (*p++ != ';') return YT_MALFORMED;
        spaces(&p); if (!*p) return YT_MALFORMED;
    }
    *out = tmp; return YT_OK;
}
Validation tableau_validate(const Partition *shape, const Tableau *t, bool decreasing)
{
    Validation v = {true,true,true,true};
    int n = partition_size(shape); bool seen[YT_CELLS+1] = {false};
    if (shape->count != t->shape.count) v.shape = false;
    for (int r = 0; r < t->shape.count; ++r) {
        if (r >= shape->count || t->shape.rows[r] != shape->rows[r]) v.shape = false;
        for (int c = 0; c < t->shape.rows[r]; ++c) {
            int entry = t->entries[r][c];
            if (entry < 1 || entry > n || seen[entry]) v.standard = false;
            else seen[entry] = true;
            if (c && (decreasing ? t->entries[r][c-1] <= entry : t->entries[r][c-1] >= entry)) v.rows = false;
            if (r && c < t->shape.rows[r-1] && (decreasing ? t->entries[r-1][c] <= entry : t->entries[r-1][c] >= entry)) v.columns = false;
        }
    }
    for (int i = 1; i <= n; ++i) if (!seen[i]) v.standard = false;
    v.standard = v.standard && v.shape && v.rows && v.columns;
    return v;
}
MathStatus permutation_rsk(const char *text, Tableau *pout, Tableau *qout)
{
    int values[YT_DIM], n; const char *cursor = text;
    MathStatus s = list(&cursor,values,&n,false);
    if (s != YT_OK) return s;
    spaces(&cursor); if (*cursor) return YT_MALFORMED;
    bool seen[YT_DIM+1] = {false};
    for (int i = 0; i < n; ++i) {
        if (values[i] < 1 || values[i] > n || seen[values[i]]) return YT_MALFORMED;
        seen[values[i]] = true;
    }
    Tableau p = {0}, q = {0};
    for (int i = 0; i < n; ++i) {
        int value = values[i];
        for (int r = 0; r < YT_DIM; ++r) {
            int c = 0;
            while (c < p.shape.rows[r] && p.entries[r][c] < value) ++c;
            if (c == p.shape.rows[r]) {
                p.entries[r][c] = value; q.entries[r][c] = i+1;
                ++p.shape.rows[r]; q.shape.rows[r] = p.shape.rows[r];
                if (r == p.shape.count) ++p.shape.count;
                q.shape.count = p.shape.count;
                break;
            }
            int bumped = p.entries[r][c]; p.entries[r][c] = value; value = bumped;
        }
    }
    *pout = p; *qout = q; return YT_OK;
}
