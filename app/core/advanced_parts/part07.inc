static void rational_normalize(Rational *value)
{
    if(!value->numerator) { value->denominator=1; return; }
    uint64_t g=gcd_u64(abs_i64(value->numerator),value->denominator);
    value->numerator/=(int64_t)g; value->denominator/=g;
}

static bool mul_i64(int64_t a,int64_t b,int64_t *out)
{
    if(!a || !b) { *out=0; return true; }
    if(a==INT64_MIN && b==-1) return false;
    if(b==INT64_MIN && a==-1) return false;
    if(a>0) {
        if(b>0 && a>INT64_MAX/b) return false;
        if(b<0 && b<INT64_MIN/a) return false;
    } else {
        if(b>0 && a<INT64_MIN/b) return false;
        if(b<0 && a!=0 && b<INT64_MAX/a) return false;
    }
    *out=a*b; return true;
}

static bool rational_multiply(Rational left,Rational right,Rational *out)
{
    uint64_t g1=gcd_u64(abs_i64(left.numerator),right.denominator);
    uint64_t g2=gcd_u64(abs_i64(right.numerator),left.denominator);
    int64_t a=left.numerator/(int64_t)g1,b=right.numerator/(int64_t)g2;
    uint64_t d1=left.denominator/g2,d2=right.denominator/g1;
    int64_t numerator;
    if(!mul_i64(a,b,&numerator) || d1>UINT64_MAX/d2) return false;
    *out=(Rational){numerator,d1*d2}; rational_normalize(out); return true;
}

static bool rational_add(Rational left,Rational right,Rational *out)
{
    uint64_t g=gcd_u64(left.denominator,right.denominator);
    uint64_t lm=right.denominator/g,rm=left.denominator/g;
    int64_t a,b;
    if(lm>(uint64_t)INT64_MAX || rm>(uint64_t)INT64_MAX) return false;
    if(!mul_i64(left.numerator,(int64_t)lm,&a) || !mul_i64(right.numerator,(int64_t)rm,&b)) return false;
    if((b>0 && a>INT64_MAX-b) || (b<0 && a<INT64_MIN-b)) return false;
    if(left.denominator>UINT64_MAX/lm) return false;
    *out=(Rational){a+b,left.denominator*lm}; rational_normalize(out); return true;
}

static void sort_partition(Partition *partition)
{
    for(int i=1;i<partition->count;++i) {
        int value=partition->rows[i],j=i;
        while(j>0 && partition->rows[j-1]<value) { partition->rows[j]=partition->rows[j-1]; --j; }
        partition->rows[j]=value;
    }
}

static bool power_add(PowerExpansion *expansion,const Partition *index,Rational coefficient)
{
    if(!coefficient.numerator) return true;
    for(int i=0;i<expansion->count;++i) if(same_partition(&expansion->terms[i].index,index)) {
        Rational sum;
        if(!rational_add(expansion->terms[i].coefficient,coefficient,&sum)) return false;
        expansion->terms[i].coefficient=sum; return true;
    }
    if(expansion->count>=YT_RESULT_TERMS) return false;
    expansion->terms[expansion->count++]=(PowerTerm){*index,coefficient};
    return true;
}

static bool power_multiply(const PowerExpansion *left,const PowerExpansion *right,PowerExpansion *out)
{
    PowerExpansion result={0};
    for(int i=0;i<left->count;++i) for(int j=0;j<right->count;++j) {
        Partition index=left->terms[i].index;
        if(index.count+right->terms[j].index.count>YT_DIM) return false;
        for(int k=0;k<right->terms[j].index.count;++k) index.rows[index.count++]=right->terms[j].index.rows[k];
        sort_partition(&index);
        Rational coefficient;
        if(!rational_multiply(left->terms[i].coefficient,right->terms[j].coefficient,&coefficient)) return false;
        if(!power_add(&result,&index,coefficient)) return false;
    }
    *out=result; return true;
}

static MathStatus schur_power_expansion(const Partition *shape,PowerExpansion *out)
{
    int degree=partition_size(shape);
    PartitionList partitions;
    MathStatus status=collect_partitions(degree,&partitions); if(status!=YT_OK) return status;
    PowerExpansion result={0};
    for(int i=0;i<partitions.count;++i) {
        int64_t character=0;
        status=symmetric_character_cycle_type(shape,&partitions.values[i],&character);
        if(status!=YT_OK) return status;
        if(!character) continue;
        bool overflow=false; uint64_t z=z_partition(&partitions.values[i],&overflow);
        if(overflow || !z) return YT_OVERFLOW;
        if(!power_add(&result,&partitions.values[i],(Rational){character,z})) return YT_OVERFLOW;
    }
    *out=result; return YT_OK;
}

static MathStatus plethysm_power_factor(const Partition *inner,int scale,PowerExpansion *out)
{
    PowerExpansion base;
    MathStatus status=schur_power_expansion(inner,&base); if(status!=YT_OK) return status;
    PowerExpansion result={0};
    for(int i=0;i<base.count;++i) {
        Partition index=base.terms[i].index;
        for(int j=0;j<index.count;++j) {
            if(index.rows[j]>YT_CELLS/scale) return YT_LIMIT;
            index.rows[j]*=scale;
        }
        if(!power_add(&result,&index,base.terms[i].coefficient)) return YT_OVERFLOW;
    }
    *out=result; return YT_OK;
}

MathStatus schur_plethysm_text(const Partition *outer,const Partition *inner,
                               char *out,size_t out_size)
{
    if(!outer || !inner || !out || !out_size) return YT_MALFORMED;
    MathStatus status=partition_validate(outer); if(status!=YT_OK) return status;
    status=partition_validate(inner); if(status!=YT_OK) return status;
    int outer_degree=partition_size(outer),inner_degree=partition_size(inner);
    int degree=outer_degree*inner_degree;
    if(degree>YT_PLETHYSM_DEGREE_MAX) return YT_COMPLEXITY;
    out[0]=0;
    if(!outer_degree) { snprintf(out,out_size,"1"); return YT_OK; }
    if(!inner_degree) { snprintf(out,out_size,"0"); return YT_OK; }

    PartitionList outer_cycles;
    status=collect_partitions(outer_degree,&outer_cycles); if(status!=YT_OK) return status;
    PowerExpansion total={0};
    for(int a=0;a<outer_cycles.count;++a) {
        int64_t outer_character=0;
        status=symmetric_character_cycle_type(outer,&outer_cycles.values[a],&outer_character);
        if(status!=YT_OK) return status;
        if(!outer_character) continue;
        bool overflow=false; uint64_t z=z_partition(&outer_cycles.values[a],&overflow);
        if(overflow || !z) return YT_OVERFLOW;
        Rational outer_coefficient={outer_character,z};
        PowerExpansion product={.count=1};
        product.terms[0]=(PowerTerm){{0},{1,1}};
        for(int part=0;part<outer_cycles.values[a].count;++part) {
            PowerExpansion factor,next;
            status=plethysm_power_factor(inner,outer_cycles.values[a].rows[part],&factor);
            if(status!=YT_OK) return status;
            if(!power_multiply(&product,&factor,&next)) return YT_OVERFLOW;
            product=next;
        }
        for(int i=0;i<product.count;++i) {
            Rational coefficient;
            if(!rational_multiply(product.terms[i].coefficient,outer_coefficient,&coefficient)) return YT_OVERFLOW;
            if(!power_add(&total,&product.terms[i].index,coefficient)) return YT_OVERFLOW;
        }
    }

