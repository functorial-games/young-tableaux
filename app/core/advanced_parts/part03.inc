static void lr_search(LRState *state,int index)
{
    if(state->overflow) return;
    if(index==state->cell_count) {
        if(state->count==UINT64_MAX) { state->overflow=true; return; }
        ++state->count;
        if(state->list) {
            if(state->list->count>=YT_RESULT_TABLEAUX) state->list_overflow=true;
            else state->list->values[state->list->count++]=state->current;
        }
        return;
    }
    Cell cell=state->cells[index];
    int row=cell.row-1,column=cell.column-1;
    if(row<0 || row>=YT_DIM || column<0 || column>=YT_DIM) { state->overflow=true; return; }
    int right=0;
    if(column+1<state->outer.rows[row]
       && column+1>=partition_row_local(&state->inner,row))
        right=state->current.entries[row][column+1];
    int above=0;
    if(row>0 && column>=partition_row_local(&state->inner,row-1)
       && column<state->outer.rows[row-1])
        above=state->current.entries[row-1][column];
    for(int value=1;value<=state->content.count;++value) {
        if(state->used[value]>=state->target[value]) continue;
        if(right && value>right) continue;
        if(above && above>=value) continue;
        state->current.entries[row][column]=value;
        ++state->used[value];
        if(lr_prefix_lattice(state)) lr_search(state,index+1);
        --state->used[value];
        state->current.entries[row][column]=0;
    }
}

static MathStatus lr_prepare(const Partition *inner,const Partition *content,
                             const Partition *outer,LRState *state)
{
    if(!inner || !content || !outer || !state) return YT_MALFORMED;
    MathStatus status=partition_validate(inner); if(status!=YT_OK) return status;
    status=partition_validate(content); if(status!=YT_OK) return status;
    status=partition_validate(outer); if(status!=YT_OK) return status;
    if(!partition_contains(outer,inner)) return YT_MALFORMED;
    int skew_size=partition_size(outer)-partition_size(inner);
    if(partition_size(content)!=skew_size) return YT_MALFORMED;
    if(skew_size>YT_ADVANCED_DEGREE_MAX) return YT_COMPLEXITY;
    memset(state,0,sizeof(*state));
    state->inner=*inner; state->content=*content; state->outer=*outer;
    state->current.outer=*outer; state->current.inner=*inner;
    for(int value=1;value<=content->count;++value) state->target[value]=content->rows[value-1];
    /* Reverse row word: top to bottom, right to left. */
    for(int row=0;row<outer->count;++row) {
        int start=partition_row_local(inner,row);
        for(int column=outer->rows[row]-1;column>=start;--column)
            state->cells[state->cell_count++]=(Cell){row+1,column+1};
    }
    return YT_OK;
}

MathStatus littlewood_richardson_coefficient(const Partition *inner,
                                              const Partition *content,
                                              const Partition *outer,
                                              uint64_t *out)
{
    if(!out) return YT_MALFORMED;
    LRState state;
    MathStatus status=lr_prepare(inner,content,outer,&state);
    if(status!=YT_OK) return status;
    lr_search(&state,0);
    if(state.overflow) return YT_OVERFLOW;
    *out=state.count;
    return YT_OK;
}

MathStatus littlewood_richardson_tableaux(const Partition *inner,
                                           const Partition *content,
                                           const Partition *outer,
                                           SkewTableauList *out)
{
    if(!out) return YT_MALFORMED;
    LRState state;
    MathStatus status=lr_prepare(inner,content,outer,&state);
    if(status!=YT_OK) return status;
    SkewTableauList result={0};
    state.list=&result;
    lr_search(&state,0);
    if(state.overflow) return YT_OVERFLOW;
    if(state.list_overflow) return YT_COMPLEXITY;
    *out=result;
    return YT_OK;
}

/* ---------- partition enumeration ---------- */

typedef bool (*PartitionVisitor)(const Partition *partition,void *context);

static bool enumerate_partition_rec(int remaining,int maximum,Partition *current,
                                    PartitionVisitor visitor,void *context)
{
    if(!remaining) return visitor(current,context);
    if(current->count>=YT_DIM) return true;
    int top=remaining<maximum?remaining:maximum;
    for(int part=top;part>=1;--part) {
        current->rows[current->count++]=part;
        if(!enumerate_partition_rec(remaining-part,part,current,visitor,context)) {
            --current->count; return false;
        }
        --current->count;
    }
    return true;
}

static bool enumerate_partitions(int n,PartitionVisitor visitor,void *context)
{
    if(n<0 || n>YT_CELLS) return false;
    if(n==0) { Partition empty={0}; return visitor(&empty,context); }
    Partition current={0};
    return enumerate_partition_rec(n,n,&current,visitor,context);
}

static uint64_t gcd_u64(uint64_t a,uint64_t b)
{
    while(b) { uint64_t t=a%b; a=b; b=t; }
    return a;
}

static uint64_t factorial_small(int n,bool *overflow)
{
    uint64_t value=1;
    for(int k=2;k<=n;++k) {
        if(value>UINT64_MAX/(uint64_t)k) { *overflow=true; return 0; }
        value*= (uint64_t)k;
    }
    return value;
}

static uint64_t z_partition(const Partition *partition,bool *overflow)
{
    uint64_t value=1;
    for(int i=0;i<partition->count;) {
        int part=partition->rows[i],multiplicity=0;
        while(i<partition->count && partition->rows[i]==part) { ++multiplicity; ++i; }
        for(int k=0;k<multiplicity;++k) {
            if(value>UINT64_MAX/(uint64_t)part) { *overflow=true; return 0; }
            value*= (uint64_t)part;
        }
        uint64_t fact=factorial_small(multiplicity,overflow);
        if(*overflow || value>UINT64_MAX/fact) { *overflow=true; return 0; }
        value*=fact;
    }
    return value;
}

/* ---------- Schur product ---------- */

typedef struct {
    const Partition *left,*right;
    char *out; size_t capacity;
    int terms;
    MathStatus status;
} SchurProductContext;

static bool schur_product_visit(const Partition *outer,void *raw)
{
    SchurProductContext *context=raw;
    if(!partition_contains(outer,context->left)) return true;
    uint64_t coefficient=0;
    MathStatus status=littlewood_richardson_coefficient(context->left,context->right,outer,&coefficient);
    if(status!=YT_OK) { context->status=status; return false; }
    if(!coefficient) return true;
    if(context->terms++) text_append(context->out,context->capacity," + ");
    if(coefficient!=1) text_append(context->out,context->capacity,"%" PRIu64 " ",coefficient);
    text_append(context->out,context->capacity,"s_");
    partition_to_text(outer,context->out,context->capacity);
    return true;
}

