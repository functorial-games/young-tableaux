typedef struct {
    Partition shape,content;
    int target[YT_DIM+1],used[YT_DIM+1];
    Tableau tableau;
    uint64_t count;
    bool overflow;
} KostkaState;

static void kostka_search(KostkaState *state,int linear)
{
    if(state->overflow) return;
    int total=partition_size(&state->shape);
    if(linear==total) {
        if(state->count==UINT64_MAX) { state->overflow=true; return; }
        ++state->count; return;
    }
    int offset=0,row=0;
    while(row<state->shape.count && linear>=offset+state->shape.rows[row]) {
        offset+=state->shape.rows[row]; ++row;
    }
    int column=linear-offset;
    int left=column?state->tableau.entries[row][column-1]:0;
    int above=(row && column<state->shape.rows[row-1])?state->tableau.entries[row-1][column]:0;
    for(int value=1;value<=state->content.count;++value) {
        if(state->used[value]>=state->target[value]) continue;
        if(left && left>value) continue;
        if(above && above>=value) continue;
        state->tableau.entries[row][column]=value;
        ++state->used[value];
        kostka_search(state,linear+1);
        --state->used[value];
        state->tableau.entries[row][column]=0;
    }
}

static MathStatus kostka_number(const Partition *shape,const Partition *content,uint64_t *out)
{
    if(!shape || !content || !out) return YT_MALFORMED;
    MathStatus status=partition_validate(shape); if(status!=YT_OK) return status;
    status=partition_validate(content); if(status!=YT_OK) return status;
    if(partition_size(shape)!=partition_size(content)) return YT_MALFORMED;
    if(partition_size(shape)>YT_ADVANCED_DEGREE_MAX) return YT_LIMIT;
    KostkaState state={0}; state.shape=*shape; state.content=*content; state.tableau.shape=*shape;
    for(int value=1;value<=content->count;++value) state.target[value]=content->rows[value-1];
    kostka_search(&state,0);
    if(state.overflow) return YT_OVERFLOW;
    *out=state.count; return YT_OK;
}

static bool collect_partition_visit(const Partition *partition,void *raw)
{
    PartitionList *list=raw;
    if(list->count>=YT_RESULT_PARTITIONS) return false;
    list->values[list->count++]=*partition;
    return true;
}

static MathStatus collect_partitions(int n,PartitionList *out)
{
    if(!out || n<0) return YT_MALFORMED;
    if(n>YT_ADVANCED_DEGREE_MAX) return YT_COMPLEXITY;
    PartitionList result={0};
    if(!enumerate_partitions(n,collect_partition_visit,&result)) return YT_LIMIT;
    *out=result; return YT_OK;
}

static bool equal_text_ci(const char *a,const char *b)
{
    if(!a || !b) return false;
    while(*a && *b) {
        unsigned char ca=(unsigned char)*a++,cb=(unsigned char)*b++;
        if(tolower(ca)!=tolower(cb)) return false;
    }
    return !*a && !*b;
}

static void append_fraction(char *out,size_t capacity,int64_t numerator,uint64_t denominator)
{
    if(!numerator) return;
    if(denominator==1) text_append(out,capacity,"%" PRId64,numerator);
    else text_append(out,capacity,"%" PRId64 "÷%" PRIu64,numerator,denominator);
}

static void append_jacobi_trudi(const Partition *shape,bool elementary,char *out,size_t capacity)
{
    Partition index=elementary?partition_conjugate(shape):*shape;
    const char *symbol=elementary?"e":"h";
    int length=index.count;
    text_append(out,capacity,"det(");
    for(int row=0;row<length;++row) {
        if(row) text_append(out,capacity,"; ");
        for(int column=0;column<length;++column) {
            if(column) text_append(out,capacity,", ");
            int k=index.rows[row]-(row+1)+(column+1);
            if(k<0) text_append(out,capacity,"0");
            else if(k==0) text_append(out,capacity,"1");
            else text_append(out,capacity,"%s_%d",symbol,k);
        }
    }
    text_append(out,capacity,")");
}

typedef struct {
    const Partition *shape;
    char *out; size_t capacity;
    int terms;
    MathStatus status;
} BasisExpansionContext;

static bool monomial_basis_visit(const Partition *content,void *raw)
{
    BasisExpansionContext *context=raw;
    uint64_t coefficient=0;
    MathStatus status=kostka_number(context->shape,content,&coefficient);
    if(status!=YT_OK) { context->status=status; return false; }
    if(!coefficient) return true;
    if(context->terms++) text_append(context->out,context->capacity," + ");
    if(coefficient!=1) text_append(context->out,context->capacity,"%" PRIu64 " ",coefficient);
    text_append(context->out,context->capacity,"m_");
    partition_to_text(content,context->out,context->capacity);
    return true;
}

static bool power_basis_visit(const Partition *cycle_type,void *raw)
{
    BasisExpansionContext *context=raw;
    int64_t character=0;
    MathStatus status=symmetric_character_cycle_type(context->shape,cycle_type,&character);
    if(status!=YT_OK) { context->status=status; return false; }
    if(!character) return true;
    bool overflow=false; uint64_t z=z_partition(cycle_type,&overflow);
    if(overflow || !z) { context->status=YT_OVERFLOW; return false; }
    if(context->terms++) text_append(context->out,context->capacity,character<0?" − ":" + ");
    else if(character<0) text_append(context->out,context->capacity,"−");
    int64_t magnitude=character<0?-character:character;
    if(magnitude!=1 || z!=1) {
        append_fraction(context->out,context->capacity,magnitude,z);
        text_append(context->out,context->capacity," ");
    }
    text_append(context->out,context->capacity,"p_");
    partition_to_text(cycle_type,context->out,context->capacity);
    return true;
}

