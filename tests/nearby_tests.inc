#include "nearby.h"
#include <complex.h>
#include <math.h>

static Control *nearby_control(Controls *u,int id)
{
    for(int i=0;i<u->count;++i) if(u->controls[i].id==id) return &u->controls[i];
    CHECK(false); return NULL;
}
static Control *maybe_nearby_control(Controls *u,int id)
{
    for(int i=0;i<u->count;++i) if(u->controls[i].id==id) return &u->controls[i];
    return NULL;
}
static void choose_shape(Console *c,const char *text)
{
    snprintf(c->fields[SET_LAMBDA],512,"%s",text);
    console_run(c,DisplayPartition);
}
static void activate(Console *c,Controls *u,int id)
{ console_event(c,u,(ControlEvent){EVENT_ACTIVATE,id}); }

static void nearby_tests(void)
{
    Console *c=calloc(1,sizeof(*c)); Controls *u=calloc(1,sizeof(*u)); CHECK(c && u);
    console_init(c); controls_init(u);
    CHECK(c->wegert.half_height==1.5 && c->wegert.center_real==0.0);

    console_layout(c,u,576,1152);
    for(int row=0;row<=3;++row) CHECK(maybe_nearby_control(u,ADDABLE_BASE+row));
    for(int row=0;row<3;++row) CHECK(maybe_nearby_control(u,REMOVABLE_BASE+row));
    activate(c,u,ADDABLE_BASE); CHECK(strcmp(c->fields[SET_LAMBDA],"4,2,1")==0);
    CHECK(c->shape_history_count==1);
    CHECK(c->wegert.half_height==1.5 && c->scripted_facts[0]==0);
    activate(c,u,SHAPE_UNDO); CHECK(strcmp(c->fields[SET_LAMBDA],"3,2,1")==0);
    CHECK(c->shape_history_count==0);
    activate(c,u,ADDABLE_BASE+3); CHECK(strcmp(c->fields[SET_LAMBDA],"3,2,1,1")==0);
    activate(c,u,SHAPE_RESET); CHECK(strcmp(c->fields[SET_LAMBDA],"3,2,1")==0);

    choose_shape(c,"2,2"); console_layout(c,u,576,1152);
    CHECK(maybe_nearby_control(u,ADDABLE_BASE));
    CHECK(!maybe_nearby_control(u,ADDABLE_BASE+1));
    CHECK(maybe_nearby_control(u,ADDABLE_BASE+2));
    CHECK(!maybe_nearby_control(u,REMOVABLE_BASE));
    CHECK(maybe_nearby_control(u,REMOVABLE_BASE+1));
    activate(c,u,ADDABLE_BASE+1); CHECK(strcmp(c->fields[SET_LAMBDA],"2,2")==0);
    activate(c,u,ADDABLE_BASE); CHECK(strcmp(c->fields[SET_LAMBDA],"3,2")==0);
    activate(c,u,SHAPE_UNDO); CHECK(strcmp(c->fields[SET_LAMBDA],"2,2")==0);
    activate(c,u,REMOVABLE_BASE+1); CHECK(strcmp(c->fields[SET_LAMBDA],"2,1")==0);
    CHECK(c->shape_history_count==0);

    choose_shape(c,"[]"); console_layout(c,u,576,1152);
    CHECK(maybe_nearby_control(u,ADDABLE_BASE));
    CHECK(!maybe_nearby_control(u,REMOVABLE_BASE));
    activate(c,u,ADDABLE_BASE); CHECK(strcmp(c->fields[SET_LAMBDA],"1")==0);
    activate(c,u,SHAPE_UNDO); CHECK(strcmp(c->fields[SET_LAMBDA],"[]")==0);

    choose_shape(c,"bad"); activate(c,u,ADDABLE_BASE);
    CHECK(strcmp(c->fields[SET_LAMBDA],"bad")==0);

    choose_shape(c,"64"); console_layout(c,u,576,1152);
    CHECK(!maybe_nearby_control(u,ADDABLE_BASE));
    CHECK(maybe_nearby_control(u,ADDABLE_BASE+1));
    activate(c,u,ADDABLE_BASE); CHECK(strcmp(c->fields[SET_LAMBDA],"64")==0);

    choose_shape(c,"64,64,64,64"); console_layout(c,u,576,1152);
    for(int row=0;row<=4;++row) CHECK(!maybe_nearby_control(u,ADDABLE_BASE+row));

    choose_shape(c,"2,1"); console_layout(c,u,576,1152);
    Control *zoom=nearby_control(u,PLOT_ZOOM_IN);
    int plot_bottom=0, plot_index=-1, derived_index=-1;
    for(int i=0;i<u->count;++i) {
        if(u->controls[i].kind==WEGERT) {
            plot_bottom=u->controls[i].rect.y+u->controls[i].rect.h;
            plot_index=i;
        }
        if(!strcmp(u->controls[i].text,"Derived facts")) derived_index=i;
        CHECK(!strstr(u->controls[i].text,"conjugate blocks"));
        CHECK(!strstr(u->controls[i].text,"WEGERT PLOT"));
    }
    CHECK(plot_index>=0 && derived_index>plot_index);
    CHECK(zoom->rect.y==plot_bottom+4*u->scale);
    controls_scroll(u,zoom->rect.y);
    int x=zoom->rect.x+zoom->rect.w/2,y=zoom->rect.y-u->scroll+zoom->rect.h/2;
    controls_touch(u,TOUCH_DOWN,4,x,y);
    ControlEvent e=controls_touch(u,TOUCH_UP,4,x,y); CHECK(e.id==PLOT_ZOOM_IN);
    console_event(c,u,e); CHECK(c->wegert.half_height==1.0);
    controls_touch(u,TOUCH_DOWN,4,x,y);
    controls_touch(u,TOUCH_MOVE,4,x,y-80);
    e=controls_touch(u,TOUCH_UP,4,x,y-80); CHECK(e.kind==EVENT_NONE);
    console_event(c,u,e); CHECK(c->wegert.half_height==1.0);
    activate(c,u,PLOT_RIGHT); activate(c,u,PLOT_UP);
    CHECK(fabs(c->wegert.center_real-0.3)<1e-12 && fabs(c->wegert.center_imag-0.3)<1e-12);
    activate(c,u,ADDABLE_BASE); CHECK(c->wegert.half_height==1.0 && c->wegert.center_real>0.0);
    for(int i=0;i<100;++i) activate(c,u,PLOT_ZOOM_IN);
    CHECK(c->wegert.half_height==0.015);
    for(int i=0;i<100;++i) activate(c,u,PLOT_ZOOM_OUT);
    CHECK(c->wegert.half_height==6.0);
    for(int i=0;i<100;++i) { activate(c,u,PLOT_LEFT); activate(c,u,PLOT_DOWN); }
    CHECK(c->wegert.center_real==-8.0 && c->wegert.center_imag==-8.0);
    for(int i=0;i<100;++i) { activate(c,u,PLOT_RIGHT); activate(c,u,PLOT_UP); }
    CHECK(c->wegert.center_real==8.0 && c->wegert.center_imag==8.0);
    activate(c,u,PLOT_RESET);
    CHECK(c->wegert.center_real==0.0 && c->wegert.center_imag==0.0 && c->wegert.half_height==1.5);

    choose_shape(c,"2,1");
    double complex points[]={0.2+0.3*I,-0.6+0.4*I,1.3+0.7*I,-5.0-2.0*I,8.0+8.0*I};
    for(unsigned i=0;i<sizeof(points)/sizeof(points[0]);++i) {
        double complex z=points[i], value=z/((1-z)*(1-z)*(1-z*z*z));
        double phase,log_modulus;
        CHECK(wegert_phase_log(&c->wegert,creal(z),cimag(z),&phase,&log_modulus));
        CHECK(fabs(remainder(phase-carg(value),6.28318530717958647692))<1e-10);
        CHECK(fabs(log_modulus-log(cabs(value)))<1e-10);
    }
    double phase,log_modulus;
    CHECK(!wegert_phase_log(&c->wegert,0,0,&phase,&log_modulus));
    CHECK(!wegert_phase_log(&c->wegert,1,0,&phase,&log_modulus));
    CHECK(!wegert_phase_log(&c->wegert,NAN,0,&phase,&log_modulus));
    choose_shape(c,"[]");
    CHECK(wegert_phase_log(&c->wegert,0,0,&phase,&log_modulus));
    CHECK(phase==0.0 && log_modulus==0.0);
    choose_shape(c,"64,64,64,64");
    CHECK(wegert_phase_log(&c->wegert,1e100,1e100,&phase,&log_modulus));
    CHECK(isfinite(phase) && isfinite(log_modulus));

    char maximum[512]="4";
    for(int i=1;i<64;++i) strcat(maximum,",4");
    choose_shape(c,maximum); console_layout(c,u,576,1152);
    for(int row=0;row<64;++row) CHECK(!maybe_nearby_control(u,ADDABLE_BASE+row));

    const int widths[]={320,576,720,1152};
    for(unsigned index=0;index<sizeof(widths)/sizeof(widths[0]);++index) {
        console_layout(c,u,widths[index],1152);
        CHECK(u->count<UI_MAX);
        bool ids[OP_BASE+OP_COUNT]={false};
        for(int i=0;i<u->count;++i) {
            const Control *control=&u->controls[i];
            CHECK(control->rect.x>=0 && control->rect.x+control->rect.w<=u->width);
            CHECK(!strstr(control->text,"LUA") && !strstr(control->text,"SCRIPTED"));
            CHECK(!strstr(control->text,"conjugate blocks"));
            if(control->id) { CHECK(!ids[control->id]); ids[control->id]=true; }
        }
        for(int op=0;op<OP_COUNT;++op) CHECK(ids[OP_BASE+op]);
        CHECK(ids[RSK_START] && ids[RSK_PREV] && ids[RSK_NEXT] && ids[RSK_END]);
        CHECK(ids[JDT_RESET] && ids[JDT_STEP] && ids[JDT_SLIDE] && ids[JDT_RECTIFY]);
    }

    choose_shape(c,"3,2,1"); c->french=true; console_layout(c,u,576,1152);
    CHECK(nearby_control(u,ADDABLE_BASE+2)->rect.y<nearby_control(u,ADDABLE_BASE)->rect.y);
    c->french=false; console_layout(c,u,576,1152);
    CHECK(nearby_control(u,ADDABLE_BASE+2)->rect.y>nearby_control(u,ADDABLE_BASE)->rect.y);

    /* Hit targets may overlay non-interactive block projections, but distinct
       interactive controls must not cover one another. */
    for(int i=0;i<u->count;++i) for(int j=i+1;j<u->count;++j) {
        if(!u->controls[i].id || !u->controls[j].id) continue;
        Rect a=u->controls[i].rect,b=u->controls[j].rect;
        CHECK(a.x+a.w<=b.x || b.x+b.w<=a.x || a.y+a.h<=b.y || b.y+b.h<=a.y);
    }
    free(c); free(u);
}
