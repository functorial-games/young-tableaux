#include "rsk.h"
#include "parse.h"
#include <string.h>
static MathStatus sequence_parse(const char *text,int *values,int *count,bool permutation)
{
    const char *cursor=text;
    MathStatus status=parse_integer_list(&cursor,values,count,false);
    if(status!=YT_OK) return status;
    parse_spaces(&cursor);
    if(*cursor) return YT_MALFORMED;
    bool seen[YT_DIM+1]={false};
    for(int index=0;index<*count;++index) {
        int value=values[index];
        if(value<1) return YT_MALFORMED;
        if(permutation) {
            if(value>*count || seen[value]) return YT_MALFORMED;
            seen[value]=true;
        }
    }
    return YT_OK;
}

MathStatus permutation_parse(const char *text,Permutation *out)
{
    if(!text || !out) return YT_MALFORMED;
    Permutation result={0};
    MathStatus status=sequence_parse(text,result.values,&result.count,true);
    if(status==YT_OK) *out=result;
    return status;
}

MathStatus word_parse(const char *text,Word *out)
{
    if(!text || !out) return YT_MALFORMED;
    Word result={0};
    MathStatus status=sequence_parse(text,result.letters,&result.count,false);
    if(status==YT_OK) *out=result;
    return status;
}

static MathStatus record_insertion(Tableau *recording,Cell new_cell,int record)
{
    Partition grown;
    MathStatus status=partition_add_cell(&recording->shape,new_cell,&grown);
    if(status!=YT_OK) return status;
    recording->shape=grown;
    recording->entries[new_cell.row-1][new_cell.column-1]=record;
    return YT_OK;
}

static MathStatus rsk_sequence(const int *values,const int *records,int count,int step,RSKTrace *out)
{
    RSKTrace trace={0};
    if(!values || !out || count<0 || count>YT_DIM) return YT_MALFORMED;
    trace.count=count;
    memcpy(trace.values,values,(size_t)count*sizeof(*values));
    if(step<0) step=0;
    if(step>trace.count) step=trace.count;
    trace.step=step;
    trace.complete=step==trace.count;
    for(int index=0;index<step;++index) {
        InsertionTrace insertion;
        MathStatus status=tableau_row_insert_trace(&trace.p,trace.values[index],&trace.p,&insertion);
        if(status!=YT_OK) return status;
        status=record_insertion(&trace.q,insertion.new_cell,records?records[index]:index+1);
        if(status!=YT_OK) return status;
        if(index+1==step) {
            trace.path_count=insertion.count;
            memcpy(trace.path,insertion.cells,sizeof(trace.path));
            trace.inserted=trace.values[index];
        }
    }
    *out=trace;
    return YT_OK;
}

MathStatus rsk_permutation(const Permutation *input,int step,RSKTrace *out)
{
    if(!input || input->count<0 || input->count>YT_DIM) return YT_MALFORMED;
    bool seen[YT_DIM+1]={false};
    for(int index=0;index<input->count;++index) {
        int value=input->values[index];
        if(value<1 || value>input->count || seen[value]) return YT_MALFORMED;
        seen[value]=true;
    }
    return rsk_sequence(input->values,NULL,input->count,step,out);
}

MathStatus rsk_word(const Word *input,int step,RSKTrace *out)
{
    if(!input || input->count<0 || input->count>YT_DIM) return YT_MALFORMED;
    for(int index=0;index<input->count;++index)
        if(input->letters[index]<1) return YT_MALFORMED;
    return rsk_sequence(input->letters,NULL,input->count,step,out);
}

static bool ordered_biletters(const Biword *input)
{
    if(input->count<0 || input->count>YT_DIM) return false;
    for(int i=0;i<input->count;++i) {
        Biletter letter=input->letters[i];
        if(letter.top<1 || letter.bottom<1) return false;
        if(i && (letter.top<input->letters[i-1].top ||
            (letter.top==input->letters[i-1].top && letter.bottom<input->letters[i-1].bottom))) return false;
    }
    return true;
}

MathStatus biword_parse(const char *text,Biword *out)
{
    if(!text || !out) return YT_MALFORMED;
    int top[YT_DIM],bottom[YT_DIM],top_count,bottom_count;
    const char *cursor=text;
    MathStatus status=parse_integer_list(&cursor,top,&top_count,true);
    if(status!=YT_OK) return status;
    parse_spaces(&cursor);
    if(*cursor++!=';') return YT_MALFORMED;
    status=parse_integer_list(&cursor,bottom,&bottom_count,false);
    if(status!=YT_OK) return status;
    parse_spaces(&cursor);
    if(*cursor || top_count!=bottom_count) return YT_MALFORMED;
    Biword result={.count=top_count};
    for(int i=0;i<top_count;++i) result.letters[i]=(Biletter){top[i],bottom[i]};
    if(!ordered_biletters(&result)) return YT_MALFORMED;
    *out=result;
    return YT_OK;
}

MathStatus nat_matrix_parse(const char *text,NatMatrix *out)
{
    if(!text || !out) return YT_MALFORMED;
    NatMatrix result={0};
    const char *cursor=text;
    parse_spaces(&cursor);
    if(!*cursor) { *out=result; return YT_OK; }
    for(;;) {
        if(result.row_count==YT_DIM) return YT_LIMIT;
        int count;
        MathStatus status=parse_integer_list(&cursor,result.entries[result.row_count],&count,true);
        if(status!=YT_OK) return status;
        if(!count || (result.row_count && count!=result.column_count)) return YT_MALFORMED;
        result.column_count=count;
        for(int i=0;i<count;++i) if(result.entries[result.row_count][i]<0) return YT_MALFORMED;
        ++result.row_count;
        parse_spaces(&cursor);
        if(!*cursor) break;
        if(*cursor++!=';') return YT_MALFORMED;
        parse_spaces(&cursor);
        if(!*cursor) return YT_MALFORMED;
    }
    *out=result;
    return YT_OK;
}

MathStatus rsk_biword(const Biword *input,int step,RSKTrace *out)
{
    if(!input || !ordered_biletters(input)) return YT_MALFORMED;
    int values[YT_DIM],records[YT_DIM];
    for(int i=0;i<input->count;++i) {
        values[i]=input->letters[i].bottom;
        records[i]=input->letters[i].top;
    }
    return rsk_sequence(values,records,input->count,step,out);
}

static MathStatus matrix_to_biword(const NatMatrix *matrix,Biword *out)
{
    if(matrix->row_count<0 || matrix->row_count>YT_DIM || matrix->column_count<0 || matrix->column_count>YT_DIM ||
       ((matrix->row_count==0)!=(matrix->column_count==0))) return YT_MALFORMED;
    Biword result={0};
    for(int row=0;row<matrix->row_count;++row) for(int column=0;column<matrix->column_count;++column) {
        int multiplicity=matrix->entries[row][column];
        if(multiplicity<0) return YT_MALFORMED;
        if(multiplicity>YT_DIM-result.count) return YT_LIMIT;
        for(int copy=0;copy<multiplicity;++copy) result.letters[result.count++]=(Biletter){row+1,column+1};
    }
    *out=result;
    return YT_OK;
}

MathStatus rsk_matrix(const NatMatrix *input,int step,RSKTrace *out)
{
    if(!input) return YT_MALFORMED;
    Biword biword;
    MathStatus status=matrix_to_biword(input,&biword);
    return status==YT_OK?rsk_biword(&biword,step,out):status;
}

MathStatus permutation_rsk_trace(const char *text,int step,RSKTrace *out)
{
    Permutation input;
    MathStatus status=permutation_parse(text,&input);
    return status==YT_OK?rsk_permutation(&input,step,out):status;
}

MathStatus word_rsk_trace(const char *text,int step,RSKTrace *out)
{
    Word input;
    MathStatus status=word_parse(text,&input);
    return status==YT_OK?rsk_word(&input,step,out):status;
}

MathStatus permutation_rsk(const char *text,Tableau *pout,Tableau *qout)
{
    RSKTrace trace;
    MathStatus status=permutation_rsk_trace(text,YT_DIM,&trace);
    if(status!=YT_OK) return status;
    *pout=trace.p;
    *qout=trace.q;
    return YT_OK;
}

MathStatus word_rsk(const char *text,Tableau *pout,Tableau *qout)
{
    RSKTrace trace;
    MathStatus status=word_rsk_trace(text,YT_DIM,&trace);
    if(status!=YT_OK) return status;
    *pout=trace.p;
    *qout=trace.q;
    return YT_OK;
}
