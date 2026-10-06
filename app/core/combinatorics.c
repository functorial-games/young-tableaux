#include "advanced.h"
#include <string.h>

static MathStatus permutation_validate(const Permutation *input)
{
    if(!input || input->count<0 || input->count>YT_DIM) return YT_MALFORMED;
    bool seen[YT_DIM+1]={false};
    for(int index=0;index<input->count;++index) {
        int value=input->values[index];
        if(value<1 || value>input->count || seen[value]) return YT_MALFORMED;
        seen[value]=true;
    }
    return YT_OK;
}

MathStatus inverse_rsk(const PermutationRSKResult *input,Permutation *out)
{
    if(!input || !out) return YT_MALFORMED;
    StandardTableau checked;
    const Tableau *insertion=&input->insertion.filling,*recording=&input->recording.filling;
    if(tableau_as_standard(insertion,&checked)!=YT_OK || tableau_as_standard(recording,&checked)!=YT_OK)
        return YT_MALFORMED;
    if(insertion->shape.count!=recording->shape.count || memcmp(insertion->shape.rows,recording->shape.rows,(size_t)insertion->shape.count*sizeof(int))) return YT_MALFORMED;
    int count=partition_size(&insertion->shape);
    if(count>YT_DIM) return YT_LIMIT;
    Tableau work=*insertion,labels=*recording;
    Permutation result={.count=count};
    for(int label=count;label>0;--label) {
        Cell corner={0};
        for(int row=0;row<labels.shape.count;++row)
            for(int column=0;column<labels.shape.rows[row];++column)
                if(labels.entries[row][column]==label) corner=(Cell){row+1,column+1};
        Tableau smaller;
        MathStatus status=tableau_reverse_insert(&work,corner,&smaller,&result.values[label-1]);
        if(status!=YT_OK) return status;
        Partition shape;
        status=partition_remove_cell(&labels.shape,corner,&shape);
        if(status!=YT_OK) return status;
        labels.shape=shape; work=smaller;
    }
    if(permutation_validate(&result)!=YT_OK) return YT_MALFORMED;
    *out=result; return YT_OK;
}

/* Deleting 1 uses the existing forward jeu owner, then relabels 2..n. */
static MathStatus delete_one(const StandardTableau *input,StandardTableau *out,Cell *corner)
{
    StandardTableau checked;
    if(!input || tableau_as_standard(&input->filling,&checked)!=YT_OK || !partition_size(&input->filling.shape)) return YT_MALFORMED;
    JeuState slide={0};
    slide.filling.shape.outer=input->filling.shape;
    slide.filling.shape.inner=(Partition){.count=1,.rows={1}};
    memcpy(slide.filling.entries,input->filling.entries,sizeof(slide.filling.entries));
    slide.filling.entries[0][0]=0;
    MathStatus status=jeu_slide(&slide,(Cell){1,1});
    if(status!=YT_OK) return status;
    Tableau smaller={.shape=slide.filling.shape.outer};
    memcpy(smaller.entries,slide.filling.entries,sizeof(smaller.entries));
    for(int row=0;row<smaller.shape.count;++row)
        for(int column=0;column<smaller.shape.rows[row];++column) --smaller.entries[row][column];
    status=tableau_as_standard(&smaller,out);
    if(status==YT_OK) *corner=slide.hole;
    return status;
}

MathStatus tableau_promote(const StandardTableau *input,StandardTableau *out)
{
    StandardTableau checked,result;
    if(!input || !out || tableau_as_standard(&input->filling,&checked)!=YT_OK) return YT_MALFORMED;
    int count=partition_size(&input->filling.shape);
    if(!count) { *out=checked; return YT_OK; }
    Cell corner;
    MathStatus status=delete_one(input,&result,&corner);
    if(status!=YT_OK) return status;
    result.filling.shape=input->filling.shape;
    result.filling.entries[corner.row-1][corner.column-1]=count;
    return tableau_as_standard(&result.filling,out);
}

MathStatus tableau_evacuate(const StandardTableau *input,StandardTableau *out)
{
    StandardTableau work;
    if(!input || !out || tableau_as_standard(&input->filling,&work)!=YT_OK) return YT_MALFORMED;
    Tableau result={.shape=input->filling.shape};
    for(int label=partition_size(&result.shape);label>0;--label) {
        StandardTableau smaller; Cell corner;
        MathStatus status=delete_one(&work,&smaller,&corner);
        if(status!=YT_OK) return status;
        result.entries[corner.row-1][corner.column-1]=label; work=smaller;
    }
    return tableau_as_standard(&result,out);
}

static MathStatus branch(const Partition *input,bool grow,PartitionList *out)
{
    if(!out) return YT_MALFORMED;
    MathStatus status=partition_validate(input); if(status!=YT_OK) return status;
    Cell corners[YT_DIM+1];
    int count=grow?partition_addable(input,corners):partition_removable(input,corners);
    if(count>YT_RESULT_PARTITIONS) return YT_COMPLEXITY;
    PartitionList result={0};
    for(int index=0;index<count;++index) {
        status=grow?partition_add_cell(input,corners[index],&result.values[index]):partition_remove_cell(input,corners[index],&result.values[index]);
        if(status!=YT_OK) return status;
        ++result.count;
    }
    *out=result; return YT_OK;
}
MathStatus partition_branch_up(const Partition *input,PartitionList *out) { return branch(input,true,out); }
MathStatus partition_branch_down(const Partition *input,PartitionList *out) { return branch(input,false,out); }

static bool contains(const Partition *outer,const Partition *inner)
{
    if(inner->count>outer->count) return false;
    for(int row=0;row<inner->count;++row) if(inner->rows[row]>outer->rows[row]) return false;
    return true;
}
static MathStatus path_search(const Partition *end,PartitionPath *path,PartitionPathList *list)
{
    const Partition *last=&path->values[path->count-1];
    if(partition_size(last)==partition_size(end)) {
        if(list->count==YT_RESULT_PATHS) return YT_COMPLEXITY;
        list->values[list->count++]=*path; return YT_OK;
    }
    PartitionList successors;
    MathStatus status=partition_branch_up(last,&successors);
    if(status!=YT_OK) return status;
    for(int index=0;index<successors.count;++index) if(contains(end,&successors.values[index])) {
        path->values[path->count++]=successors.values[index];
        status=path_search(end,path,list); --path->count;
        if(status!=YT_OK) return status;
    }
    return YT_OK;
}
MathStatus young_graph_paths(const Partition *start,const Partition *end,PartitionPathList *out)
{
    if(!out) return YT_MALFORMED;
    MathStatus status=partition_validate(start); if(status!=YT_OK) return status;
    status=partition_validate(end); if(status!=YT_OK) return status;
    if(!contains(end,start)) return YT_MALFORMED;
    if(partition_size(end)-partition_size(start)>=YT_PATH_LENGTH_MAX) return YT_COMPLEXITY;
    PartitionPathList result={0}; PartitionPath path={.count=1}; path.values[0]=*start;
    status=path_search(end,&path,&result);
    if(status==YT_OK) *out=result;
    return status;
}

/* SplitMix64: an explicit reproducible, noncryptographic stream. */
uint64_t young_rng_next(YoungRNG *rng)
{
    uint64_t value=(rng->state+=UINT64_C(0x9e3779b97f4a7c15));
    value=(value^(value>>30))*UINT64_C(0xbf58476d1ce4e5b9);
    value=(value^(value>>27))*UINT64_C(0x94d049bb133111eb);
    return value^(value>>31);
}
static uint64_t draw_below(YoungRNG *rng,uint64_t bound)
{
    uint64_t threshold=(UINT64_C(0)-bound)%bound,value;
    do { value=young_rng_next(rng); } while(value<threshold);
    return value%bound;
}
MathStatus random_permutation(int count,YoungRNG *rng,Permutation *out)
{
    if(!rng || !out || count<0) return YT_MALFORMED;
    if(count>YT_DIM) return YT_LIMIT;
    YoungRNG stream=*rng; Permutation result={.count=count};
    for(int index=0;index<count;++index) result.values[index]=index+1;
    for(int index=count-1;index>0;--index) {
        int selected=(int)draw_below(&stream,(uint64_t)index+1);
        int value=result.values[index]; result.values[index]=result.values[selected]; result.values[selected]=value;
    }
    *rng=stream; *out=result; return YT_OK;
}
MathStatus random_standard_tableau(const Partition *shape,YoungRNG *rng,StandardTableau *out)
{
    if(!rng || !out) return YT_MALFORMED;
    MathStatus status=partition_validate(shape); if(status!=YT_OK) return status;
    Tableau result={.shape=*shape}; Partition current=*shape; YoungRNG stream=*rng;
    for(int label=partition_size(shape);label>0;--label) {
        Cell corners[YT_DIM]; int count=partition_removable(&current,corners);
        uint64_t weights[YT_DIM],total=0;
        for(int index=0;index<count;++index) {
            Partition smaller;
            status=partition_remove_cell(&current,corners[index],&smaller); if(status!=YT_OK) return status;
            status=partition_standard_count(&smaller,&weights[index]); if(status!=YT_OK) return status;
            if(total>UINT64_MAX-weights[index]) return YT_OVERFLOW;
            total+=weights[index];
        }
        uint64_t draw=draw_below(&stream,total); int selected=0;
        while(draw>=weights[selected]) { draw-=weights[selected]; ++selected; }
        Cell corner=corners[selected]; result.entries[corner.row-1][corner.column-1]=label;
        status=partition_remove_cell(&current,corner,&current); if(status!=YT_OK) return status;
    }
    StandardTableau checked; status=tableau_as_standard(&result,&checked);
    if(status==YT_OK) { *rng=stream; *out=checked; }
    return status;
}
MathStatus sample_plancherel_partition(int count,YoungRNG *rng,Partition *out)
{
    if(!rng || !out) return YT_MALFORMED;
    YoungRNG stream=*rng; Permutation permutation;
    MathStatus status=random_permutation(count,&stream,&permutation); if(status!=YT_OK) return status;
    RSKTrace trace; status=rsk_permutation(&permutation,YT_DIM,&trace);
    if(status==YT_OK) { *rng=stream; *out=trace.p.shape; }
    return status;
}

MathStatus permutation_cycle_type(const Permutation *input,Partition *out)
{
    if(!out || permutation_validate(input)!=YT_OK) return YT_MALFORMED;
    bool seen[YT_DIM]={false}; Partition result={0};
    for(int start=0;start<input->count;++start) if(!seen[start]) {
        int index=start,length=0;
        do { seen[index]=true; ++length; index=input->values[index]-1; } while(index!=start);
        result.rows[result.count++]=length;
    }
    for(int index=1;index<result.count;++index) {
        int value=result.rows[index],position=index;
        while(position && result.rows[position-1]<value) { result.rows[position]=result.rows[position-1]; --position; }
        result.rows[position]=value;
    }
    *out=result; return YT_OK;
}
static MathStatus subsequence(const Permutation *input,bool increasing,NumberList *out)
{
    if(!out || permutation_validate(input)!=YT_OK) return YT_MALFORMED;
    int lengths[YT_DIM]={0},previous[YT_DIM],best=-1;
    for(int index=0;index<input->count;++index) {
        lengths[index]=1; previous[index]=-1;
        for(int earlier=0;earlier<index;++earlier)
            if((increasing?input->values[earlier]<input->values[index]:input->values[earlier]>input->values[index]) && lengths[earlier]+1>lengths[index]) {
                lengths[index]=lengths[earlier]+1; previous[index]=earlier;
            }
        if(best<0 || lengths[index]>lengths[best]) best=index;
    }
    NumberList result={.count=best<0?0:lengths[best]};
    for(int position=result.count-1;position>=0;--position) { result.values[position]=input->values[best]; best=previous[best]; }
    *out=result; return YT_OK;
}
MathStatus permutation_lis(const Permutation *input,NumberList *out) { return subsequence(input,true,out); }
MathStatus permutation_lds(const Permutation *input,NumberList *out) { return subsequence(input,false,out); }
MathStatus permutation_coxeter_reduced_word(const Permutation *input,NumberList *out)
{
    if(!out || permutation_validate(input)!=YT_OK) return YT_MALFORMED;
    Permutation current=*input; NumberList result={0};
    for(int end=current.count;end>1;--end)
        for(int position=0;position+1<end;++position) if(current.values[position]>current.values[position+1]) {
            if(result.count==YT_CELLS) return YT_COMPLEXITY;
            result.values[result.count++]=position+1;
            int value=current.values[position]; current.values[position]=current.values[position+1]; current.values[position+1]=value;
        }
    for(int index=0;index<result.count/2;++index) {
        int value=result.values[index]; result.values[index]=result.values[result.count-1-index]; result.values[result.count-1-index]=value;
    }
    *out=result; return YT_OK;
}
MathStatus permutation_bruhat_leq(const Permutation *left,const Permutation *right,bool *out)
{
    if(!out || permutation_validate(left)!=YT_OK || permutation_validate(right)!=YT_OK || left->count!=right->count) return YT_MALFORMED;
    bool result=true;
    for(int prefix=1;prefix<=left->count;++prefix)
        for(int threshold=1;threshold<=left->count;++threshold) {
            int first=0,second=0;
            for(int position=0;position<prefix;++position) { first+=left->values[position]>=threshold; second+=right->values[position]>=threshold; }
            if(first>second) result=false;
        }
    *out=result; return YT_OK;
}
