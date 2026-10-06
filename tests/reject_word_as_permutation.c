#include "rsk.h"
void reject_word_as_permutation(void)
{
    Word word={2,{1,1}};
    RSKTrace result;
    rsk_permutation(&word,2,&result);
}
