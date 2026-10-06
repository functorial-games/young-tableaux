#include "advanced.h"
#include "console.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static unsigned checks;
#define CHECK(condition) do { ++checks; if(!(condition)) { fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#condition); exit(1); } } while(0)
static bool same_partition(const Partition *left,const Partition *right)
{ return left->count==right->count && !memcmp(left->rows,right->rows,(size_t)left->count*sizeof(int)); }
static bool same_tableau(const StandardTableau *left,const StandardTableau *right)
{
    if(!same_partition(&left->filling.shape,&right->filling.shape)) return false;
    for(int row=0;row<left->filling.shape.count;++row)
        for(int column=0;column<left->filling.shape.rows[row];++column)
            if(left->filling.entries[row][column]!=right->filling.entries[row][column]) return false;
    return true;
}
static SymmetricFunction singleton(SymmetricBasis basis,Partition partition)
{ SymmetricFunction result={.count=1,.basis=basis}; result.terms[0]=(SymmetricTerm){partition,{1,1}}; return result; }
static Rational coefficient(const SymmetricFunction *function,const Partition *partition)
{
    for(int index=0;index<function->count;++index) if(same_partition(&function->terms[index].index,partition)) return function->terms[index].coefficient;
    return (Rational){0,1};
}
static bool same_function(const SymmetricFunction *left,const SymmetricFunction *right)
{
    if(left->basis!=right->basis) return false;
    for(int side=0;side<2;++side) {
        const SymmetricFunction *first=side?right:left,*second=side?left:right;
        for(int index=0;index<first->count;++index) {
            Rational a=first->terms[index].coefficient,b=coefficient(second,&first->terms[index].index);
            if(a.numerator*(int64_t)b.denominator!=b.numerator*(int64_t)a.denominator) return false;
        }
    }
    return true;
}
static bool next_permutation(Permutation *permutation)
{
    int position=permutation->count-2;
    while(position>=0 && permutation->values[position]>permutation->values[position+1]) --position;
    if(position<0) return false;
    int selected=permutation->count-1;
    while(permutation->values[selected]<permutation->values[position]) --selected;
    int value=permutation->values[position]; permutation->values[position]=permutation->values[selected]; permutation->values[selected]=value;
    for(int left=position+1,right=permutation->count-1;left<right;++left,--right) {
        value=permutation->values[left]; permutation->values[left]=permutation->values[right]; permutation->values[right]=value;
    }
    return true;
}
static int inversions(const Permutation *permutation)
{
    int count=0;
    for(int first=0;first<permutation->count;++first)
        for(int second=first+1;second<permutation->count;++second) count+=permutation->values[first]>permutation->values[second];
    return count;
}
/* Independent exponential subsequence oracle, restricted to six letters. */
static int longest_bruteforce(const Permutation *permutation,bool increasing)
{
    int best=0;
    for(unsigned mask=0;mask<(1U<<permutation->count);++mask) {
        int length=0,previous=0; bool valid=true;
        for(int index=0;index<permutation->count;++index) if(mask&(1U<<index)) {
            if(length && (increasing?previous>=permutation->values[index]:previous<=permutation->values[index])) valid=false;
            previous=permutation->values[index]; ++length;
        }
        if(valid && length>best) best=length;
    }
    return best;
}
static void permutation_checks(void)
{
    for(int size=0;size<=6;++size) {
        Permutation permutation={.count=size};
        for(int index=0;index<size;++index) permutation.values[index]=index+1;
        do {
            RSKTrace trace; PermutationRSKResult pair; Permutation inverse;
            CHECK(rsk_permutation(&permutation,YT_DIM,&trace)==YT_OK);
            CHECK(rsk_permutation_result(&trace,&pair)==YT_OK);
            CHECK(inverse_rsk(&pair,&inverse)==YT_OK);
            CHECK(inverse.count==size && !memcmp(inverse.values,permutation.values,(size_t)size*sizeof(int)));
            NumberList increasing,decreasing,word;
            CHECK(permutation_lis(&permutation,&increasing)==YT_OK);
            CHECK(permutation_lds(&permutation,&decreasing)==YT_OK);
            CHECK(increasing.count==longest_bruteforce(&permutation,true));
            CHECK(decreasing.count==longest_bruteforce(&permutation,false));
            CHECK(increasing.count==(size?trace.p.shape.rows[0]:0));
            CHECK(decreasing.count==trace.p.shape.count);
            CHECK(permutation_coxeter_reduced_word(&permutation,&word)==YT_OK);
            CHECK(word.count==inversions(&permutation));
            Permutation rebuilt={.count=size};
            for(int index=0;index<size;++index) rebuilt.values[index]=index+1;
            for(int index=0;index<word.count;++index) {
                int position=word.values[index]-1,value=rebuilt.values[position];
                rebuilt.values[position]=rebuilt.values[position+1]; rebuilt.values[position+1]=value;
            }
            CHECK(!memcmp(rebuilt.values,permutation.values,(size_t)size*sizeof(int)));
            StandardTableau evacuated,twice,promoted,first,second,third;
            CHECK(tableau_evacuate(&pair.recording,&evacuated)==YT_OK);
            CHECK(tableau_evacuate(&evacuated,&twice)==YT_OK);
            CHECK(same_tableau(&pair.recording,&twice));
            CHECK(tableau_promote(&pair.recording,&promoted)==YT_OK);
            CHECK(tableau_evacuate(&promoted,&first)==YT_OK);
            CHECK(tableau_promote(&first,&second)==YT_OK);
            CHECK(tableau_evacuate(&pair.recording,&third)==YT_OK);
            CHECK(same_tableau(&second,&third));
            bool rectangle=true;
            for(int row=1;row<trace.q.shape.count;++row) rectangle=rectangle && trace.q.shape.rows[row]==trace.q.shape.rows[0];
            if(rectangle) {
                first=pair.recording;
                for(int iteration=0;iteration<size;++iteration) { CHECK(tableau_promote(&first,&second)==YT_OK); first=second; }
                CHECK(same_tableau(&first,&pair.recording));
            }
        } while(next_permutation(&permutation));
    }
}
static PartitionList partitions[9];
static void collect_partitions_rec(int remaining,int maximum,Partition *current,PartitionList *list)
{
    if(!remaining) { CHECK(list->count<YT_RESULT_PARTITIONS); list->values[list->count++]=*current; return; }
    for(int part=remaining<maximum?remaining:maximum;part>0;--part) {
        current->rows[current->count++]=part;
        collect_partitions_rec(remaining-part,part,current,list); --current->count;
    }
}
static uint64_t factorial(int number)
{ uint64_t result=1; for(int value=2;value<=number;++value) result*=(unsigned)value; return result; }
static uint64_t centralizer(const Partition *partition)
{
    uint64_t result=1;
    for(int index=0;index<partition->count;) {
        int value=partition->rows[index],count=0;
        while(index<partition->count && partition->rows[index]==value) { result*=(unsigned)value; ++index; ++count; }
        result*=factorial(count);
    }
    return result;
}
static void character_checks(void)
{
    const int64_t s3[3][3]={{1,1,1},{-1,0,2},{1,-1,1}};
    for(int row=0;row<3;++row) for(int column=0;column<3;++column) {
        int64_t value;
        CHECK(symmetric_character_cycle_type(&partitions[3].values[row],&partitions[3].values[column],&value)==YT_OK);
        CHECK(value==s3[row][column]);
    }
    for(int size=0;size<=6;++size) {
        Partition identity={.count=size}; for(int index=0;index<size;++index) identity.rows[index]=1;
        for(int first=0;first<partitions[size].count;++first) {
            uint64_t dimension; int64_t character;
            CHECK(partition_standard_count(&partitions[size].values[first],&dimension)==YT_OK);
            CHECK(symmetric_character_cycle_type(&partitions[size].values[first],&identity,&character)==YT_OK);
            CHECK(character==(int64_t)dimension);
            for(int second=0;second<partitions[size].count;++second) {
                int64_t sum=0;
                for(int cycle=0;cycle<partitions[size].count;++cycle) {
                    int64_t left,right;
                    CHECK(symmetric_character_cycle_type(&partitions[size].values[first],&partitions[size].values[cycle],&left)==YT_OK);
                    CHECK(symmetric_character_cycle_type(&partitions[size].values[second],&partitions[size].values[cycle],&right)==YT_OK);
                    sum+=(int64_t)(factorial(size)/centralizer(&partitions[size].values[cycle]))*left*right;
                }
                CHECK(sum==(first==second?(int64_t)factorial(size):0));
            }
        }
    }
}
/* Tiny independent rational convolution of power sums, for degree <= 6. */
static void product_oracle(const SymmetricFunction *left,const SymmetricFunction *right,SymmetricFunction *out)
{
    *out=(SymmetricFunction){.basis=BASIS_POWER};
    for(int first=0;first<left->count;++first) for(int second=0;second<right->count;++second) {
        Partition index=left->terms[first].index;
        for(int part=0;part<right->terms[second].index.count;++part) index.rows[index.count++]=right->terms[second].index.rows[part];
        for(int part=1;part<index.count;++part) { int value=index.rows[part],position=part; while(position && index.rows[position-1]<value) { index.rows[position]=index.rows[position-1]; --position; } index.rows[position]=value; }
        Rational term={left->terms[first].coefficient.numerator*right->terms[second].coefficient.numerator,left->terms[first].coefficient.denominator*right->terms[second].coefficient.denominator};
        int selected=0;
        while(selected<out->count && !same_partition(&index,&out->terms[selected].index)) ++selected;
        if(selected==out->count) { CHECK(out->count<YT_RESULT_TERMS); out->terms[out->count++]=(SymmetricTerm){index,{0,1}}; }
        Rational old=out->terms[selected].coefficient;
        int64_t numerator=old.numerator*(int64_t)term.denominator+term.numerator*(int64_t)old.denominator;
        uint64_t denominator=old.denominator*term.denominator;
        uint64_t a=(uint64_t)llabs(numerator),b=denominator;
        while(b) { uint64_t remainder=a%b; a=b; b=remainder; }
        out->terms[selected].coefficient=(Rational){numerator/(int64_t)a,denominator/a};
    }
}
static void algebra_checks(void)
{
    for(int size=0;size<=5;++size) for(int index=0;index<partitions[size].count;++index) {
        SymmetricFunction original=singleton(BASIS_SCHUR,partitions[size].values[index]),converted,back;
        for(int basis=0;basis<=4;++basis) {
            CHECK(symmetric_change_basis(&original,(SymmetricBasis)basis,&converted)==YT_OK);
            CHECK(symmetric_change_basis(&converted,BASIS_SCHUR,&back)==YT_OK);
            CHECK(same_function(&original,&back));
        }
    }
    for(int left_size=0;left_size<=3;++left_size) for(int right_size=0;right_size<=3;++right_size)
        for(int first=0;first<partitions[left_size].count;++first) for(int second=0;second<partitions[right_size].count;++second) {
            Partition left=partitions[left_size].values[first],right=partitions[right_size].values[second];
            SymmetricFunction left_s=singleton(BASIS_SCHUR,left),right_s=singleton(BASIS_SCHUR,right),left_p,right_p,actual,actual_p,expected;
            CHECK(symmetric_schur_product(&left,&right,&actual)==YT_OK);
            CHECK(symmetric_change_basis(&actual,BASIS_POWER,&actual_p)==YT_OK);
            CHECK(symmetric_change_basis(&left_s,BASIS_POWER,&left_p)==YT_OK);
            CHECK(symmetric_change_basis(&right_s,BASIS_POWER,&right_p)==YT_OK);
            product_oracle(&left_p,&right_p,&expected);
            CHECK(same_function(&actual_p,&expected));
            for(int term=0;term<actual.count;++term) {
                SkewTableauList *tableaux=malloc(sizeof(*tableaux)); CHECK(tableaux);
                uint64_t count;
                CHECK(littlewood_richardson_coefficient(&left,&right,&actual.terms[term].index,&count)==YT_OK);
                CHECK(littlewood_richardson_tableaux(&left,&right,&actual.terms[term].index,tableaux)==YT_OK);
                CHECK(count==(uint64_t)tableaux->count && count==(uint64_t)actual.terms[term].coefficient.numerator);
                for(int tableau=0;tableau<tableaux->count;++tableau) CHECK(skew_tableau_validate(&tableaux->values[tableau]).semistandard);
                free(tableaux);
            }
        }
    SymmetricFunction row=singleton(BASIS_SCHUR,(Partition){.count=1,.rows={2}}),answer;
    CHECK(symmetric_plethysm(&row,&row,&answer)==YT_OK);
    Rational value=coefficient(&answer,&(Partition){.count=1,.rows={4}}); CHECK(value.numerator==1 && value.denominator==1);
    value=coefficient(&answer,&(Partition){.count=2,.rows={2,2}}); CHECK(value.numerator==1 && value.denominator==1); CHECK(answer.count==2);
    SymmetricFunction unit=singleton(BASIS_SCHUR,(Partition){.count=1,.rows={1}});
    CHECK(symmetric_plethysm(&unit,&row,&answer)==YT_OK && same_function(&answer,&row));
    CHECK(symmetric_plethysm(&row,&unit,&answer)==YT_OK && same_function(&answer,&row));
    SymmetricFunction constant=singleton(BASIS_SCHUR,(Partition){0});
    CHECK(symmetric_plethysm(&row,&constant,&answer)==YT_OK && same_function(&answer,&constant));
    SymmetricFunction column=singleton(BASIS_SCHUR,(Partition){.count=2,.rows={1,1}});
    CHECK(symmetric_plethysm(&column,&constant,&answer)==YT_OK && answer.count==0);
    const int alphabet[]={1,2};
    CHECK(symmetric_specialize(&row,alphabet,2,&value)==YT_OK); CHECK(value.numerator==7 && value.denominator==1);
    CHECK(symmetric_specialize(&column,alphabet,2,&value)==YT_OK); CHECK(value.numerator==2 && value.denominator==1);
    CHECK(symmetric_parse("0;1,2;2;1,2;2",&answer)==YT_OK && same_function(&answer,&row));
    CHECK(symmetric_parse("1;1;1;1;[]",&answer)==YT_OK);
    CHECK(symmetric_plethysm(&answer,&row,&answer)==YT_OK);
    CHECK(coefficient(&answer,&row.terms[0].index).numerator==1);
    CHECK(coefficient(&answer,&(Partition){0}).numerator==1);
    CHECK(symmetric_parse("0;1,0;2",&answer)==YT_MALFORMED);
    CHECK(symmetric_parse("0;1;9",&answer)==YT_COMPLEXITY);
    CHECK(symmetric_parse("0;1;2;",&answer)!=YT_OK);
    CHECK(symmetric_parse("0;1",&answer)!=YT_OK);
    SymmetricFunction large=row;
    large.terms[0].coefficient=(Rational){INT64_MAX,1};
    CHECK(symmetric_specialize(&large,alphabet,2,&value)==YT_OVERFLOW);
    uint64_t zero=99;
    CHECK(littlewood_richardson_coefficient(&(Partition){.count=1,.rows={3}},&(Partition){.count=1,.rows={1}},&(Partition){.count=1,.rows={2}},&zero)==YT_OK && zero==0);
    CHECK(littlewood_richardson_coefficient(&(Partition){.count=1,.rows={1}},&(Partition){.count=1,.rows={1}},&(Partition){.count=1,.rows={3}},&zero)==YT_OK && zero==0);
    char tiny[2]="x"; CHECK(symmetric_text(&row,tiny,sizeof(tiny))==YT_LIMIT && !strcmp(tiny,"x"));
}
static void graph_checks(void)
{
    Partition empty={0};
    PartitionPathList *example=malloc(sizeof(*example)); CHECK(example);
    CHECK(young_graph_paths(&(Partition){.count=2,.rows={2,1}},&(Partition){.count=2,.rows={3,2}},example)==YT_OK && example->count==2);
    example->count=77;
    CHECK(young_graph_paths(&empty,&(Partition){.count=3,.rows={4,3,2}},example)==YT_COMPLEXITY && example->count==77);
    free(example);
    for(int size=0;size<=6;++size) for(int index=0;index<partitions[size].count;++index) {
        Partition shape=partitions[size].values[index]; uint64_t dimension;
        PartitionPathList *paths=malloc(sizeof(*paths)); CHECK(paths);
        CHECK(partition_standard_count(&shape,&dimension)==YT_OK);
        CHECK(young_graph_paths(&empty,&shape,paths)==YT_OK);
        CHECK((uint64_t)paths->count==dimension);
        for(int path=0;path<paths->count;++path) {
            Tableau tableau={.shape=shape};
            CHECK(paths->values[path].count==size+1);
            for(int step=1;step<=size;++step) {
                const Partition *before=&paths->values[path].values[step-1],*after=&paths->values[path].values[step];
                CHECK(partition_size(after)==partition_size(before)+1);
                for(int row=0;row<after->count;++row) if(after->rows[row]!=(row<before->count?before->rows[row]:0)) tableau.entries[row][after->rows[row]-1]=step;
            }
            StandardTableau checked; CHECK(tableau_as_standard(&tableau,&checked)==YT_OK);
        }
        free(paths);
        PartitionList up,down;
        CHECK(partition_branch_up(&shape,&up)==YT_OK);
        for(int next=0;next<up.count;++next) {
            CHECK(partition_branch_down(&up.values[next],&down)==YT_OK);
            bool found=false; for(int previous=0;previous<down.count;++previous) found=found || same_partition(&shape,&down.values[previous]); CHECK(found);
        }
    }
}
static void bruhat_checks(void)
{
    Permutation permutations[24]; int count=0;
    Permutation current={.count=4,.values={1,2,3,4}};
    do { permutations[count++]=current; } while(next_permutation(&current));
    bool reach[24][24]={{false}};
    /* Independent strong-order cover graph: any position transposition
     * increasing inversion length by exactly one, then transitive closure. */
    for(int first=0;first<count;++first) {
        reach[first][first]=true;
        for(int a=0;a<4;++a) for(int b=a+1;b<4;++b) {
            Permutation next=permutations[first]; int value=next.values[a]; next.values[a]=next.values[b]; next.values[b]=value;
            if(inversions(&next)!=inversions(&permutations[first])+1) continue;
            for(int second=0;second<count;++second) if(!memcmp(next.values,permutations[second].values,4*sizeof(int))) reach[first][second]=true;
        }
    }
    for(int middle=0;middle<count;++middle) for(int first=0;first<count;++first) for(int second=0;second<count;++second) reach[first][second]=reach[first][second] || (reach[first][middle] && reach[middle][second]);
    for(int first=0;first<count;++first) for(int second=0;second<count;++second) {
        bool actual; CHECK(permutation_bruhat_leq(&permutations[first],&permutations[second],&actual)==YT_OK); CHECK(actual==reach[first][second]);
    }
}
static void random_checks(void)
{
    YoungRNG rng={.state=73}; int permutation_counts[6]={0},tableau_counts[2]={0},shape_counts[3]={0};
    Partition shape={.count=2,.rows={2,1}};
    for(int iteration=0;iteration<12000;++iteration) {
        Permutation permutation; StandardTableau tableau; Partition sampled;
        CHECK(random_permutation(3,&rng,&permutation)==YT_OK);
        Permutation current={.count=3,.values={1,2,3}}; int index=0;
        while(memcmp(current.values,permutation.values,3*sizeof(int))) { CHECK(next_permutation(&current)); ++index; }
        ++permutation_counts[index];
        CHECK(random_standard_tableau(&shape,&rng,&tableau)==YT_OK);
        CHECK(same_partition(&shape,&tableau.filling.shape));
        ++tableau_counts[tableau.filling.entries[0][1]==2?0:1];
        CHECK(sample_plancherel_partition(3,&rng,&sampled)==YT_OK);
        if(sampled.count==1) ++shape_counts[0]; else if(sampled.count==2) ++shape_counts[1]; else { CHECK(sampled.count==3); ++shape_counts[2]; }
    }
    for(int index=0;index<6;++index) CHECK(abs(permutation_counts[index]-2000)<300);
    for(int index=0;index<2;++index) CHECK(abs(tableau_counts[index]-6000)<500);
    CHECK(abs(shape_counts[0]-2000)<300 && abs(shape_counts[1]-8000)<500 && abs(shape_counts[2]-2000)<300);
    Permutation permutation; uint64_t before=rng.state;
    CHECK(random_permutation(-1,&rng,&permutation)==YT_MALFORMED && before==rng.state);
    CHECK(random_permutation(65,&rng,&permutation)==YT_LIMIT && before==rng.state);
}
static void dispatch_checks(void)
{
    Console *console=malloc(sizeof(*console)); Controls *controls=malloc(sizeof(*controls)); CHECK(console && controls);
    for(int operation=0;operation<OP_COUNT;++operation) {
        console_init(console); controls_init(controls);
        strcpy(console->fields[SET_LAMBDA],"2,1"); strcpy(console->fields[SET_MU],"1"); strcpy(console->fields[SET_NU],"3,1");
        strcpy(console->fields[SET_TABLEAU],"1,3;2,4"); strcpy(console->fields[SET_RECORDING],"1,2;3,4");
        strcpy(console->fields[SET_CELL],"2,1");
        if(operation==JeuDeTaquinSlide || operation==Rectify) console->jeu_ok=false;
        console_event(console,controls,(ControlEvent){EVENT_ACTIVATE,OP_BASE+operation});
        int section=operation_info[operation].section;
        CHECK(console->output[section][0]);
        CHECK(!strstr(console->output[section],"NOT IMPLEMENTED") && !strstr(console->output[section],"INTERNAL DISPATCH"));
        if(section>=3) {
            console_layout(console,controls,576,1152); bool rendered=false;
            for(int index=0;index<controls->count;++index) if(controls->controls[index].kind==OUTPUT && !strcmp(controls->controls[index].text,console->output[section])) rendered=true;
            CHECK(rendered);
        }
    }
    /* Invalid inputs still produce visible feedback in different sections. */
    const Operation errors[]={InverseRSK,LittlewoodRichardsonCoefficient,ChangeBasis,BruhatRelations};
    for(unsigned index=0;index<sizeof(errors)/sizeof(*errors);++index) {
        console_init(console);
        strcpy(console->fields[SET_TABLEAU],"bad"); strcpy(console->fields[SET_MU],"bad"); strcpy(console->fields[SET_COEFFICIENTS],"bad"); strcpy(console->fields[SET_SECOND_PERMUTATION],"bad");
        console_run(console,errors[index]); console_layout(console,controls,576,1152);
        const char *output=console->output[operation_info[errors[index]].section]; CHECK(strstr(output,"INVALID INPUT"));
        bool rendered=false; for(int control=0;control<controls->count;++control) if(controls->controls[control].kind==OUTPUT && !strcmp(controls->controls[control].text,output)) rendered=true;
        CHECK(rendered);
    }
    free(controls); free(console);
}
int main(void)
{
    for(int size=0;size<=8;++size) { Partition current={0}; collect_partitions_rec(size,size,&current,&partitions[size]); }
    permutation_checks(); character_checks(); algebra_checks(); graph_checks(); bruhat_checks(); random_checks(); dispatch_checks();
    printf("PASS %u consolidation checks: exact examples, exhaustive laws, independent algebra/order oracles, distributions, rendered dispatch\n",checks);
    return 0;
}
