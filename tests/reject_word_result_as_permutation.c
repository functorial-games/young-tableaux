#include "rsk.h"
void reject_word_result_as_permutation(const RSKTrace *trace)
{
    WordRSKResult word_result;
    (void)rsk_permutation_result(trace,&word_result);
}
