MathStatus schur_product_text(const Partition *left,const Partition *right,
                              char *out,size_t out_size)
{
    if(!left || !right || !out || !out_size) return YT_MALFORMED;
    MathStatus status=partition_validate(left); if(status!=YT_OK) return status;
    status=partition_validate(right); if(status!=YT_OK) return status;
    int degree=partition_size(left)+partition_size(right);
    if(degree>YT_ADVANCED_DEGREE_MAX) return YT_COMPLEXITY;
    out[0]=0;
    SchurProductContext context={left,right,out,out_size,0,YT_OK};
    if(!enumerate_partitions(degree,schur_product_visit,&context) && context.status==YT_OK)
        return YT_LIMIT;
    if(context.status!=YT_OK) return context.status;
    if(!context.terms) snprintf(out,out_size,"0");
    return YT_OK;
}

/* ---------- symmetric-group characters ---------- */

MathStatus permutation_cycle_type(const char *text,Partition *cycle_type)
{
    if(!text || !cycle_type) return YT_MALFORMED;
    int values[YT_DIM],count=0;
    MathStatus status=parse_permutation_values(text,values,&count);
    if(status!=YT_OK) return status;
    bool seen[YT_DIM]={false};
    int lengths[YT_DIM],cycles=0;
    for(int start=0;start<count;++start) if(!seen[start]) {
        int index=start,length=0;
        do {
            if(index<0 || index>=count || seen[index]) {
                if(index!=start) return YT_MALFORMED;
                break;
            }
            seen[index]=true; ++length; index=values[index]-1;
        } while(index!=start);
        lengths[cycles++]=length;
    }
    for(int i=1;i<cycles;++i) {
        int value=lengths[i],j=i;
        while(j>0 && lengths[j-1]<value) { lengths[j]=lengths[j-1]; --j; }
        lengths[j]=value;
    }
    Partition result={.count=cycles};
    for(int i=0;i<cycles;++i) result.rows[i]=lengths[i];
    *cycle_type=result;
    return YT_OK;
}

static bool skew_border_strip(const Partition *outer,const Partition *inner,
                              int expected,int *height)
{
    if(!partition_contains(outer,inner)) return false;
    Cell cells[YT_CELLS]; int count=0;
    bool occupied[YT_DIM][YT_DIM]={{false}};
    int min_row=YT_DIM,max_row=-1;
    for(int row=0;row<outer->count;++row) {
        int start=partition_row_local(inner,row);
        for(int column=start;column<outer->rows[row];++column) {
            if(count>=YT_CELLS) return false;
            cells[count++]=(Cell){row,column};
            occupied[row][column]=true;
            if(row<min_row) min_row=row;
            if(row>max_row) max_row=row;
        }
    }
    if(count!=expected || !count) return false;
    for(int row=0;row<YT_DIM-1;++row) for(int column=0;column<YT_DIM-1;++column)
        if(occupied[row][column] && occupied[row+1][column]
           && occupied[row][column+1] && occupied[row+1][column+1]) return false;
    bool reached[YT_CELLS]={false};
    int queue[YT_CELLS],head=0,tail=1; queue[0]=0; reached[0]=true;
    while(head<tail) {
        Cell a=cells[queue[head++]];
        for(int i=0;i<count;++i) if(!reached[i]) {
            Cell b=cells[i];
            int distance=abs(a.row-b.row)+abs(a.column-b.column);
            if(distance==1) { reached[i]=true; queue[tail++]=i; }
        }
    }
    if(tail!=count) return false;
    if(height) *height=max_row-min_row+1;
    return true;
}

typedef struct {
    const Partition *outer;
    int target_size,strip_size;
    int64_t sum;
    const int *cycles;
    int cycle_count,index;
    bool overflow;
} CharacterContext;

static int64_t character_rec(const Partition *shape,const int *cycles,int cycle_count,int index,bool *overflow);

static void character_subpartition_rec(const Partition *outer,int row,int previous,int remaining,
                                       Partition *current,CharacterContext *context)
{
    if(context->overflow) return;
    if(row==outer->count) {
        if(remaining) return;
        while(current->count && !current->rows[current->count-1]) --current->count;
        int height=0;
        if(!skew_border_strip(outer,current,context->strip_size,&height)) return;
        int64_t sub=character_rec(current,context->cycles,context->cycle_count,
                                  context->index+1,&context->overflow);
        if(context->overflow) return;
        if(!(height&1)) sub=-sub;
        if((sub>0 && context->sum>INT64_MAX-sub) || (sub<0 && context->sum<INT64_MIN-sub)) {
            context->overflow=true; return;
        }
        context->sum+=sub;
        return;
    }
    int max=outer->rows[row];
    if(max>previous) max=previous;
    if(max>remaining) max=remaining;
    for(int length=max;length>=0;--length) {
        int rows_left=outer->count-row-1;
        (void)rows_left;
        current->rows[row]=length;
        if(length>0 && row+1>current->count) current->count=row+1;
        character_subpartition_rec(outer,row+1,length,remaining-length,current,context);
        if(context->overflow) return;
    }
}

static int64_t character_rec(const Partition *shape,const int *cycles,int cycle_count,int index,bool *overflow)
{
    if(*overflow) return 0;
    if(index==cycle_count) return partition_size(shape)==0?1:0;
    int strip=cycles[index];
    int target=partition_size(shape)-strip;
    if(target<0) return 0;
    Partition current={0};
    CharacterContext context={shape,target,strip,0,cycles,cycle_count,index,false};
    character_subpartition_rec(shape,0,shape->count?shape->rows[0]:0,target,&current,&context);
    if(context.overflow) *overflow=true;
    return context.sum;
}

MathStatus symmetric_character_cycle_type(const Partition *shape,
                                           const Partition *cycle_type,
                                           int64_t *out)
{
    if(!shape || !cycle_type || !out) return YT_MALFORMED;
    MathStatus status=partition_validate(shape); if(status!=YT_OK) return status;
    status=partition_validate(cycle_type); if(status!=YT_OK) return status;
    int n=partition_size(shape);
    if(partition_size(cycle_type)!=n) return YT_MALFORMED;
    if(n>YT_ADVANCED_DEGREE_MAX) return YT_COMPLEXITY;
    int cycles[YT_DIM];
    for(int i=0;i<cycle_type->count;++i) cycles[i]=cycle_type->rows[i];
    bool overflow=false;
    int64_t value=character_rec(shape,cycles,cycle_type->count,0,&overflow);
    if(overflow) return YT_OVERFLOW;
    *out=value;
    return YT_OK;
}

MathStatus symmetric_character_permutation(const Partition *shape,
                                            const char *permutation,
                                            int64_t *out)
{
    Partition cycle_type;
    MathStatus status=permutation_cycle_type(permutation,&cycle_type);
    if(status!=YT_OK) return status;
    return symmetric_character_cycle_type(shape,&cycle_type,out);
}

/* ---------- Kostka numbers and symmetric-function presentation ---------- */

