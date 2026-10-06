MathStatus schur_change_basis_text(const Partition *shape,const char *basis,
                                   const char *q,const char *t,
                                   char *out,size_t out_size)
{
    if(!shape || !basis || !out || !out_size) return YT_MALFORMED;
    MathStatus status=partition_validate(shape); if(status!=YT_OK) return status;
    int n=partition_size(shape);
    if(n>YT_ADVANCED_DEGREE_MAX) return YT_COMPLEXITY;
    out[0]=0;
    if(equal_text_ci(basis,"Schur")) {
        text_append(out,out_size,"s_"); partition_to_text(shape,out,out_size); return YT_OK;
    }
    if(equal_text_ci(basis,"Monomial")) {
        BasisExpansionContext context={shape,out,out_size,0,YT_OK};
        if(!enumerate_partitions(n,monomial_basis_visit,&context) && context.status==YT_OK) return YT_LIMIT;
        if(context.status!=YT_OK) return context.status;
        if(!context.terms) snprintf(out,out_size,"0");
        return YT_OK;
    }
    if(equal_text_ci(basis,"PowerSum") || equal_text_ci(basis,"Power sum") || equal_text_ci(basis,"Power-sum")) {
        BasisExpansionContext context={shape,out,out_size,0,YT_OK};
        if(!enumerate_partitions(n,power_basis_visit,&context) && context.status==YT_OK) return YT_LIMIT;
        if(context.status!=YT_OK) return context.status;
        if(!context.terms) snprintf(out,out_size,"0");
        return YT_OK;
    }
    if(equal_text_ci(basis,"CompleteHomogeneous") || equal_text_ci(basis,"Complete homogeneous") || equal_text_ci(basis,"Complete")) {
        append_jacobi_trudi(shape,false,out,out_size); return YT_OK;
    }
    if(equal_text_ci(basis,"Elementary")) {
        append_jacobi_trudi(shape,true,out,out_size); return YT_OK;
    }
    if(equal_text_ci(basis,"HallLittlewood") || equal_text_ci(basis,"Hall-Littlewood")) {
        text_append(out,out_size,"Σ_{μ⊢%d} (K(t)⁻¹)_[",n);
        partition_to_text(shape,out,out_size);
        text_append(out,out_size,",μ] P_μ(x;t), t=%s",t&&*t?t:"t");
        return YT_OK;
    }
    if(equal_text_ci(basis,"Macdonald")) {
        text_append(out,out_size,"Σ_{μ⊢%d} (K(q,t)⁻¹)_[",n);
        partition_to_text(shape,out,out_size);
        text_append(out,out_size,",μ] P_μ(x;q,t), q=%s, t=%s",q&&*q?q:"q",t&&*t?t:"t");
        return YT_OK;
    }
    return YT_MALFORMED;
}

static MathStatus parse_variables(const char *text,char names[YT_DIM][32],int *count)
{
    if(!text || !count) return YT_MALFORMED;
    const char *cursor=text; *count=0; skip_space(&cursor);
    if(!*cursor) return YT_OK;
    while(*cursor) {
        if(*count>=YT_DIM) return YT_LIMIT;
        skip_space(&cursor);
        int length=0;
        if(!(isalpha((unsigned char)*cursor) || *cursor=='_')) return YT_MALFORMED;
        while(isalnum((unsigned char)*cursor) || *cursor=='_') {
            if(length>=30) return YT_LIMIT;
            names[*count][length++]=*cursor++;
        }
        names[*count][length]=0; ++*count;
        skip_space(&cursor);
        if(!*cursor) break;
        if(*cursor!=',') return YT_MALFORMED;
        ++cursor;
    }
    return YT_OK;
}

static const char *superscript_digit(unsigned digit)
{
    static const char *digits[]={"⁰","¹","²","³","⁴","⁵","⁶","⁷","⁸","⁹"};
    return digit<10?digits[digit]:"";
}

static void append_superscript_number(char *out,size_t capacity,int exponent)
{
    if(exponent<=1) return;
    char decimal[32]; snprintf(decimal,sizeof(decimal),"%d",exponent);
    for(const char *p=decimal;*p;++p) text_append(out,capacity,"%s",superscript_digit((unsigned)(*p-'0')));
}

typedef struct {
    const Partition *shape;
    char (*names)[32]; int variable_count,degree;
    int exponents[YT_DIM];
    char *out; size_t capacity;
    int terms;
    MathStatus status;
} SpecializationContext;

static void specialization_emit(SpecializationContext *context)
{
    int sorted[YT_DIM];
    for(int i=0;i<context->variable_count;++i) sorted[i]=context->exponents[i];
    for(int i=1;i<context->variable_count;++i) {
        int value=sorted[i],j=i;
        while(j>0 && sorted[j-1]<value) { sorted[j]=sorted[j-1]; --j; }
        sorted[j]=value;
    }
    Partition content={0};
    for(int i=0;i<context->variable_count && sorted[i]>0;++i) content.rows[content.count++]=sorted[i];
    uint64_t coefficient=0;
    MathStatus status=kostka_number(context->shape,&content,&coefficient);
    if(status!=YT_OK) { context->status=status; return; }
    if(!coefficient) return;
    if(context->terms++) text_append(context->out,context->capacity," + ");
    if(coefficient!=1) text_append(context->out,context->capacity,"%" PRIu64 " ",coefficient);
    bool any=false;
    for(int i=0;i<context->variable_count;++i) if(context->exponents[i]) {
        if(any) text_append(context->out,context->capacity,"×");
        text_append(context->out,context->capacity,"%s",context->names[i]);
        append_superscript_number(context->out,context->capacity,context->exponents[i]);
        any=true;
    }
    if(!any) text_append(context->out,context->capacity,"1");
}

static void specialization_compositions(SpecializationContext *context,int variable,int remaining)
{
    if(context->status!=YT_OK) return;
    if(variable+1==context->variable_count) {
        context->exponents[variable]=remaining;
        specialization_emit(context);
        return;
    }
    for(int exponent=0;exponent<=remaining;++exponent) {
        context->exponents[variable]=exponent;
        specialization_compositions(context,variable+1,remaining-exponent);
        if(context->status!=YT_OK) return;
    }
}

MathStatus schur_specialization_text(const Partition *shape,const char *variables,
                                     char *out,size_t out_size)
{
    if(!shape || !variables || !out || !out_size) return YT_MALFORMED;
    MathStatus status=partition_validate(shape); if(status!=YT_OK) return status;
    int degree=partition_size(shape);
    if(degree>12) return YT_COMPLEXITY;
    char names[YT_DIM][32]; int count=0;
    status=parse_variables(variables,names,&count); if(status!=YT_OK) return status;
    out[0]=0;
    if(!count) { snprintf(out,out_size,degree?"0":"1"); return YT_OK; }
    if(count>6) return YT_COMPLEXITY;
    SpecializationContext context={shape,names,count,degree,{0},out,out_size,0,YT_OK};
    specialization_compositions(&context,0,degree);
    if(context.status!=YT_OK) return context.status;
    if(!context.terms) snprintf(out,out_size,"0");
    return YT_OK;
}

/* ---------- rational power-sum engine for Schur plethysm ---------- */

typedef struct { int64_t numerator; uint64_t denominator; } Rational;
typedef struct { Partition index; Rational coefficient; } PowerTerm;
typedef struct { int count; PowerTerm terms[YT_RESULT_TERMS]; } PowerExpansion;

static uint64_t abs_i64(int64_t value)
{
    return value<0?(uint64_t)(-(value+1))+1U:(uint64_t)value;
}

