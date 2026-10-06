#include "advanced.h"
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

static void text_append(char *out,size_t capacity,const char *format,...)
{
    if(!out || !capacity) return;
    size_t used=strlen(out);
    if(used>=capacity-1) return;
    va_list args;
    va_start(args,format);
    (void)vsnprintf(out+used,capacity-used,format,args);
    va_end(args);
}

static void skip_space(const char **cursor)
{
    while(isspace((unsigned char)**cursor)) ++*cursor;
}

static MathStatus parse_integer(const char **cursor,int *out)
{
    skip_space(cursor);
    errno=0;
    char *end=NULL;
    long value=strtol(*cursor,&end,10);
    if(end==*cursor) return YT_MALFORMED;
    if(errno==ERANGE || value<INT_MIN || value>INT_MAX) return YT_OVERFLOW;
    *cursor=end;
    *out=(int)value;
    return YT_OK;
}

static MathStatus parse_number_list(const char *text,int *values,int *count,
                                    int capacity,bool positive,bool nonnegative)
{
    if(!text || !values || !count || capacity<0) return YT_MALFORMED;
    const char *cursor=text;
    *count=0;
    skip_space(&cursor);
    bool bracket=*cursor=='[';
    if(bracket) { ++cursor; skip_space(&cursor); }
    if(bracket && *cursor==']') {
        ++cursor; skip_space(&cursor);
        return *cursor?YT_MALFORMED:YT_OK;
    }
    if(!bracket && !*cursor) return YT_OK;
    for(;;) {
        if(*count>=capacity) return YT_LIMIT;
        int value=0;
        MathStatus status=parse_integer(&cursor,&value);
        if(status!=YT_OK) return status;
        if(positive && value<=0) return YT_MALFORMED;
        if(nonnegative && value<0) return YT_MALFORMED;
        values[(*count)++]=value;
        bool separated=isspace((unsigned char)*cursor)!=0;
        skip_space(&cursor);
        if(bracket && *cursor==']') {
            ++cursor; skip_space(&cursor);
            return *cursor?YT_MALFORMED:YT_OK;
        }
        if(!bracket && !*cursor) return YT_OK;
        if(*cursor==',') { ++cursor; skip_space(&cursor); separated=true; }
        if(!separated) return YT_MALFORMED;
    }
}

static MathStatus parse_permutation_values(const char *text,int *values,int *count)
{
    MathStatus status=parse_number_list(text,values,count,YT_CELLS,true,false);
    if(status!=YT_OK) return status;
    if(*count>YT_DIM) return YT_LIMIT;
    bool seen[YT_DIM+1]={false};
    for(int i=0;i<*count;++i) {
        if(values[i]>*count || seen[values[i]]) return YT_MALFORMED;
        seen[values[i]]=true;
    }
    return YT_OK;
}

static bool same_partition(const Partition *a,const Partition *b)
{
    if(!a || !b || a->count!=b->count) return false;
    for(int row=0;row<a->count;++row) if(a->rows[row]!=b->rows[row]) return false;
    return true;
}

static bool partition_contains(const Partition *outer,const Partition *inner)
{
    if(!outer || !inner || inner->count>outer->count) return false;
    for(int row=0;row<inner->count;++row)
        if(inner->rows[row]>outer->rows[row]) return false;
    return true;
}

static int partition_row_local(const Partition *partition,int row)
{
    return row>=0 && row<partition->count?partition->rows[row]:0;
}

static void partition_to_text(const Partition *partition,char *out,size_t capacity)
{
    text_append(out,capacity,"[");
    for(int row=0;row<partition->count;++row)
        text_append(out,capacity,"%s%d",row?",":"",partition->rows[row]);
    text_append(out,capacity,"]");
}

static void rsk_insert_pair(Tableau *p,Tableau *q,int value,int record)
{
    for(int row=0;row<YT_DIM;++row) {
        int column=0;
        while(column<p->shape.rows[row] && p->entries[row][column]<=value) ++column;
        if(column==p->shape.rows[row]) {
            p->entries[row][column]=value;
            q->entries[row][column]=record;
            ++p->shape.rows[row];
            q->shape.rows[row]=p->shape.rows[row];
            if(row==p->shape.count) ++p->shape.count;
            q->shape.count=p->shape.count;
            return;
        }
        int bumped=p->entries[row][column];
        p->entries[row][column]=value;
        value=bumped;
    }
}

MathStatus biword_rsk(const char *text,Tableau *p,Tableau *q)
{
    if(!text || !p || !q || p==q) return YT_MALFORMED;
    const char *separator=strchr(text,';');
    if(!separator || strchr(separator+1,';')) return YT_MALFORMED;
    char top_text[512],bottom_text[512];
    size_t top_length=(size_t)(separator-text);
    if(top_length>=sizeof(top_text) || strlen(separator+1)>=sizeof(bottom_text)) return YT_LIMIT;
    memcpy(top_text,text,top_length); top_text[top_length]=0;
    snprintf(bottom_text,sizeof(bottom_text),"%s",separator+1);
    int top[YT_CELLS],bottom[YT_CELLS],top_count=0,bottom_count=0;
    MathStatus status=parse_number_list(top_text,top,&top_count,YT_CELLS,true,false);
    if(status!=YT_OK) return status;
    status=parse_number_list(bottom_text,bottom,&bottom_count,YT_CELLS,true,false);
    if(status!=YT_OK) return status;
    if(top_count!=bottom_count) return YT_MALFORMED;
    if(top_count>YT_DIM) return YT_LIMIT;
    for(int i=1;i<top_count;++i) {
        if(top[i]<top[i-1]) return YT_MALFORMED;
        if(top[i]==top[i-1] && bottom[i]<bottom[i-1]) return YT_MALFORMED;
    }
    Tableau pp={0},qq={0};
    for(int i=0;i<top_count;++i) rsk_insert_pair(&pp,&qq,bottom[i],top[i]);
    if(!tableau_semistandard(&pp) || !tableau_semistandard(&qq)) return YT_MALFORMED;
    *p=pp; *q=qq;
    return YT_OK;
}

static MathStatus parse_matrix(const char *text,int matrix[YT_DIM][YT_DIM],int *rows,int *columns,int *total)
{
    if(!text || !rows || !columns || !total) return YT_MALFORMED;
    memset(matrix,0,sizeof(int)*YT_DIM*YT_DIM);
    *rows=0; *columns=0; *total=0;
    const char *cursor=text;
    skip_space(&cursor);
    if(!*cursor) return YT_OK;
    while(*cursor) {
        if(*rows>=YT_DIM) return YT_LIMIT;
        int count=0;
        for(;;) {
            if(count>=YT_DIM) return YT_LIMIT;
            int value=0;
            MathStatus status=parse_integer(&cursor,&value);
            if(status!=YT_OK) return status;
            if(value<0) return YT_MALFORMED;
            if(value>YT_CELLS-*total) return YT_LIMIT;
            matrix[*rows][count++]=value;
            *total+=value;
            bool had_space=isspace((unsigned char)*cursor)!=0;
            skip_space(&cursor);
            if(*cursor==',' ) { ++cursor; skip_space(&cursor); continue; }
            if(*cursor==';' || !*cursor) break;
            if(had_space) continue;
            return YT_MALFORMED;
        }
        if(!*columns) *columns=count;
        else if(count!=*columns) return YT_MALFORMED;
        ++*rows;
        if(*cursor==';') { ++cursor; skip_space(&cursor); if(!*cursor) return YT_MALFORMED; }
    }
    return YT_OK;
}

