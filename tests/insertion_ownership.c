#include "rsk.h"
#include <stdio.h>
#include <string.h>
static int insertion_calls;
MathStatus __real_tableau_row_insert_trace(const Tableau *,int,Tableau *,InsertionTrace *);
MathStatus __wrap_tableau_row_insert_trace(const Tableau *tableau,int value,Tableau *out,InsertionTrace *trace)
{
    ++insertion_calls;
    return __real_tableau_row_insert_trace(tableau,value,out,trace);
}
int main(void)
{
    RSKTrace trace={0},saved={0};
    Word word={4,{2,1,2,1}};
    Permutation permutation={4,{3,1,4,2}};
    if(rsk_word(&word,4,&trace)!=YT_OK || insertion_calls!=4) return 1;
    if(trace.p.entries[0][0]!=1 || trace.p.entries[0][1]!=1 || trace.p.entries[1][0]!=2) return 2;
    insertion_calls=0;
    if(rsk_permutation(&permutation,3,&trace)!=YT_OK || insertion_calls!=3) return 3;
    saved=trace;
    permutation.values[1]=3; /* Words may repeat; permutations may not. */
    if(rsk_permutation(&permutation,3,&trace)!=YT_MALFORMED || memcmp(&saved,&trace,sizeof(trace))) return 4;
    word.count=YT_DIM+1;
    if(rsk_word(&word,3,&trace)!=YT_MALFORMED || memcmp(&saved,&trace,sizeof(trace))) return 5;
    Biword biword;
    NatMatrix matrix;
    if(biword_parse("1,1,2;1,2,1",&biword)!=YT_OK || rsk_biword(&biword,3,&trace)!=YT_OK) return 6;
    if(trace.p.shape.rows[0]!=2 || trace.p.shape.rows[1]!=1 || trace.p.entries[0][0]!=1 ||
       trace.p.entries[0][1]!=1 || trace.p.entries[1][0]!=2 || trace.q.entries[0][1]!=1) return 7;
    RSKTrace biword_result=trace;
    bool kind_valid=true;
    if(tableau_check_kind(&trace.q,TABLEAU_STANDARD,false,&kind_valid)!=YT_OK || kind_valid) return 11;
    if(tableau_check_kind(&trace.q,TABLEAU_SEMISTANDARD,false,&kind_valid)!=YT_OK || !kind_valid) return 12;
    kind_valid=false;
    if(tableau_check_kind(&trace.q,TABLEAU_SKEW,false,&kind_valid)!=YT_UNSUPPORTED || kind_valid) return 13;
    if(nat_matrix_parse("1,1;1,0",&matrix)!=YT_OK || rsk_matrix(&matrix,3,&trace)!=YT_OK ||
       memcmp(&biword_result.p,&trace.p,sizeof(trace.p)) || memcmp(&biword_result.q,&trace.q,sizeof(trace.q))) return 8;
    if(biword_parse("1,2;1",&biword)!=YT_MALFORMED || biword_parse("1,1;2,1",&biword)!=YT_MALFORMED ||
       nat_matrix_parse("1,0;1",&matrix)!=YT_MALFORMED || nat_matrix_parse("1,-1",&matrix)!=YT_MALFORMED) return 9;
    saved=trace; matrix.entries[0][0]=YT_DIM+1;
    if(rsk_matrix(&matrix,3,&trace)!=YT_LIMIT || memcmp(&saved,&trace,sizeof(trace))) return 10;
    puts("PASS typed RSK inputs, output-preserving rejection, insertion ownership");
    return 0;
}
