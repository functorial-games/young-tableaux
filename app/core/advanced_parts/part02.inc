MathStatus matrix_rsk(const char *text,Tableau *p,Tableau *q)
{
    if(!text || !p || !q || p==q) return YT_MALFORMED;
    int matrix[YT_DIM][YT_DIM],rows=0,columns=0,total=0;
    MathStatus status=parse_matrix(text,matrix,&rows,&columns,&total);
    if(status!=YT_OK) return status;
    if(total>YT_DIM) return YT_LIMIT;
    Tableau pp={0},qq={0};
    for(int row=0;row<rows;++row)
        for(int column=0;column<columns;++column)
            for(int copy=0;copy<matrix[row][column];++copy)
                rsk_insert_pair(&pp,&qq,column+1,row+1);
    if(partition_size(&pp.shape)!=total || !tableau_semistandard(&pp) || !tableau_semistandard(&qq))
        return YT_MALFORMED;
    *p=pp; *q=qq;
    return YT_OK;
}

static bool standard_tableau(const Tableau *tableau)
{
    return tableau && tableau_validate(&tableau->shape,tableau,false).standard;
}

MathStatus inverse_rsk(const Tableau *p,const Tableau *q,NumberList *permutation)
{
    if(!p || !q || !permutation || !same_partition(&p->shape,&q->shape)
       || !standard_tableau(p) || !standard_tableau(q)) return YT_MALFORMED;
    int n=partition_size(&p->shape);
    Tableau pp=*p,qq=*q;
    NumberList result={.count=n};
    for(int record=n;record>=1;--record) {
        Cell corner={0};
        bool found=false;
        for(int row=0;row<qq.shape.count && !found;++row)
            for(int column=0;column<qq.shape.rows[row];++column)
                if(qq.entries[row][column]==record) {
                    corner=(Cell){row+1,column+1}; found=true; break;
                }
        if(!found) return YT_MALFORMED;
        int row=corner.row-1;
        if(corner.column!=qq.shape.rows[row]
           || (row+1<qq.shape.count && qq.shape.rows[row]==qq.shape.rows[row+1])) return YT_MALFORMED;
        Tableau smaller;
        int value=0;
        MathStatus status=tableau_reverse_insert(&pp,corner,&smaller,&value);
        if(status!=YT_OK) return status;
        pp=smaller;
        --qq.shape.rows[row];
        if(!qq.shape.rows[row]) --qq.shape.count;
        result.values[record-1]=value;
    }
    bool seen[YT_DIM+1]={false};
    if(n>YT_DIM) return YT_LIMIT;
    for(int i=0;i<n;++i) {
        if(result.values[i]<1 || result.values[i]>n || seen[result.values[i]]) return YT_MALFORMED;
        seen[result.values[i]]=true;
    }
    *permutation=result;
    return YT_OK;
}

MathStatus tableau_promote(const Tableau *tableau,Tableau *out)
{
    if(!tableau || !out || !standard_tableau(tableau)) return YT_MALFORMED;
    Tableau result=*tableau;
    int n=partition_size(&result.shape);
    if(!n) { *out=result; return YT_OK; }
    Cell hole={0};
    for(int row=0;row<result.shape.count && !hole.row;++row)
        for(int column=0;column<result.shape.rows[row];++column)
            if(result.entries[row][column]==1) { hole=(Cell){row+1,column+1}; break; }
    if(!hole.row) return YT_MALFORMED;
    result.entries[hole.row-1][hole.column-1]=0;
    for(;;) {
        int row=hole.row-1,column=hole.column-1;
        bool right=column+1<result.shape.rows[row];
        bool below=row+1<result.shape.count && column<result.shape.rows[row+1];
        if(!right && !below) break;
        Cell source;
        if(right && (!below || result.entries[row][column+1]<result.entries[row+1][column]))
            source=(Cell){row+1,column+2};
        else source=(Cell){row+2,column+1};
        result.entries[row][column]=result.entries[source.row-1][source.column-1];
        result.entries[source.row-1][source.column-1]=0;
        hole=source;
    }
    for(int row=0;row<result.shape.count;++row)
        for(int column=0;column<result.shape.rows[row];++column)
            if(result.entries[row][column]>0) --result.entries[row][column];
    result.entries[hole.row-1][hole.column-1]=n;
    if(!standard_tableau(&result)) return YT_MALFORMED;
    *out=result;
    return YT_OK;
}

static MathStatus tableau_delete_one(Tableau *work,Cell *removed_corner)
{
    if(!work || !removed_corner || !standard_tableau(work)) return YT_MALFORMED;
    int n=partition_size(&work->shape);
    if(!n) return YT_MALFORMED;
    Cell hole={0};
    for(int row=0;row<work->shape.count && !hole.row;++row)
        for(int column=0;column<work->shape.rows[row];++column)
            if(work->entries[row][column]==1) { hole=(Cell){row+1,column+1}; break; }
    if(!hole.row) return YT_MALFORMED;
    work->entries[hole.row-1][hole.column-1]=0;
    for(;;) {
        int row=hole.row-1,column=hole.column-1;
        bool right=column+1<work->shape.rows[row];
        bool below=row+1<work->shape.count && column<work->shape.rows[row+1];
        if(!right && !below) break;
        Cell source;
        if(right && (!below || work->entries[row][column+1]<work->entries[row+1][column]))
            source=(Cell){row+1,column+2};
        else source=(Cell){row+2,column+1};
        work->entries[row][column]=work->entries[source.row-1][source.column-1];
        work->entries[source.row-1][source.column-1]=0;
        hole=source;
    }
    *removed_corner=hole;
    int row=hole.row-1;
    --work->shape.rows[row];
    if(!work->shape.rows[row]) --work->shape.count;
    for(int r=0;r<work->shape.count;++r)
        for(int c=0;c<work->shape.rows[r];++c) --work->entries[r][c];
    if(partition_size(&work->shape) && !standard_tableau(work)) return YT_MALFORMED;
    return YT_OK;
}

MathStatus tableau_evacuate(const Tableau *tableau,Tableau *out)
{
    if(!tableau || !out || !standard_tableau(tableau)) return YT_MALFORMED;
    Tableau work=*tableau,result={0};
    result.shape=tableau->shape;
    for(int label=partition_size(&tableau->shape);label>=1;--label) {
        Cell corner={0};
        MathStatus status=tableau_delete_one(&work,&corner);
        if(status!=YT_OK) return status;
        result.entries[corner.row-1][corner.column-1]=label;
    }
    if(!standard_tableau(&result)) return YT_MALFORMED;
    *out=result;
    return YT_OK;
}

/* ---------- Littlewood–Richardson ---------- */

typedef struct {
    Partition inner,content,outer;
    Cell cells[YT_CELLS];
    int cell_count;
    int used[YT_DIM+1];
    int target[YT_DIM+1];
    SkewTableau current;
    uint64_t count;
    bool overflow;
    SkewTableauList *list;
    bool list_overflow;
} LRState;

static bool lr_prefix_lattice(const LRState *state)
{
    for(int value=1;value<YT_DIM;++value)
        if(state->used[value]<state->used[value+1]) return false;
    return true;
}

