#include "partition.h"
#include "parse.h"
#include <string.h>
MathStatus partition_parse(const char *text, Partition *out)
{
    Partition tmp = {0};
    const char *p = text;
    MathStatus s = parse_integer_list(&p, tmp.rows, &tmp.count, false);
    if (s != YT_OK) return s;
    parse_spaces(&p);
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
static bool cell_in_list(Cell cell,const Cell *cells,int count)
{
    for(int index=0;index<count;++index)
        if(cells[index].row==cell.row && cells[index].column==cell.column) return true;
    return false;
}

static MathStatus partition_value_status(const Partition *partition)
{
    if(!partition) return YT_MALFORMED;
    if(partition->count<0) return YT_MALFORMED;
    if(partition->count>YT_DIM) return YT_LIMIT;
    int size=0;
    for(int row=0;row<partition->count;++row) {
        int length=partition->rows[row];
        if(length<=0 || (row && length>partition->rows[row-1])) return YT_MALFORMED;
        if(length>YT_DIM || size>YT_CELLS-length) return YT_LIMIT;
        size+=length;
    }
    return YT_OK;
}

MathStatus partition_add_cell(const Partition *partition,Cell cell,Partition *out)
{
    if(!out) return YT_MALFORMED;
    MathStatus input_status=partition_value_status(partition);
    if(input_status!=YT_OK) return input_status;
    if(partition_size(partition)>=YT_CELLS) return YT_LIMIT;
    Cell cells[YT_DIM+1];
    int count=partition_addable(partition,cells);
    if(!cell_in_list(cell,cells,count) || cell.column>YT_DIM) return YT_MALFORMED;
    Partition result=*partition;
    int row=cell.row-1;
    if(row==result.count) {
        if(result.count>=YT_DIM) return YT_LIMIT;
        result.rows[result.count++]=1;
    } else ++result.rows[row];
    *out=result;
    return YT_OK;
}

MathStatus partition_remove_cell(const Partition *partition,Cell cell,Partition *out)
{
    if(!out) return YT_MALFORMED;
    MathStatus input_status=partition_value_status(partition);
    if(input_status!=YT_OK) return input_status;
    Cell cells[YT_DIM];
    int count=partition_removable(partition,cells);
    if(!cell_in_list(cell,cells,count)) return YT_MALFORMED;
    Partition result=*partition;
    int row=cell.row-1;
    --result.rows[row];
    if(!result.rows[row]) --result.count;
    *out=result;
    return YT_OK;
}
