#include "nearby.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

static bool row_change_allowed(const Partition *p,int row,int change)
{
    if(row<0 || row>=p->count) return false;
    if(change>0)
        return partition_size(p)<YT_CELLS && p->rows[row]<YT_DIM
            && (!row || p->rows[row]<p->rows[row-1]);
    return row+1==p->count || p->rows[row]>p->rows[row+1];
}

static void button(Controls *u,int id,const char *label,Rect rect,bool enabled)
{
    controls_add_at(u,id,enabled?BUTTON:DISABLED_BUTTON,label,rect,NULL);
}

void nearby_shape(Console *c,Controls *u)
{
    Partition p;
    if(partition_parse(c->fields[SET_LAMBDA],&p)!=YT_OK) return;
    int scale=u->scale, margin=4*scale, gap=4*scale, side=24*scale;
    int width=u->width-2*margin;
    controls_add(u,0,LABEL,"Young diagram",0,NULL);
    if(!p.count) controls_add(u,0,LABEL,"(empty diagram)",0,NULL);
    for(int visual=0;visual<p.count;++visual) {
        int row=c->french?p.count-1-visual:visual;
        int y=u->content, left=u->width-margin-2*side-gap;
        c->editable_rows[row]=(TileRowProjection){p.rows[row],p.rows[0]};
        controls_add_at(u,0,ROW_BLOCKS,"",(Rect){margin,y,left-margin-gap,side},
                        &c->editable_rows[row]);
        button(u,ROW_MINUS_BASE+row,"-",(Rect){left,y+2*scale,side,side-4*scale},
               row_change_allowed(&p,row,-1));
        button(u,ROW_PLUS_BASE+row,"+",(Rect){left+side+gap,y+2*scale,side,side-4*scale},
               row_change_allowed(&p,row,1));
        u->content=y+side;
    }
    u->content+=gap;
    button(u,ADD_ROW,"+ ROW",(Rect){margin,u->content,width,side},
           p.count<YT_DIM && partition_size(&p)<YT_CELLS);
}

void nearby_plot_controls(Console *c,Controls *u)
{
    const int zoom_ids[]={PLOT_ZOOM_OUT,PLOT_RESET,PLOT_ZOOM_IN};
    const char *zoom_labels[]={"ZOOM -","RESET","ZOOM +"};
    const int pan_ids[]={PLOT_LEFT,PLOT_UP,PLOT_DOWN,PLOT_RIGHT};
    const char *pan_labels[]={"LEFT","UP","DOWN","RIGHT"};
    int scale=u->scale, margin=4*scale, gap=4*scale;
    int width=u->width-2*margin, y=u->content, height=24*scale;
    for(int index=0;index<3;++index) {
        int left=margin+index*(width+gap)/3;
        int right=margin+(index+1)*(width+gap)/3-gap;
        bool enabled=index==1 || (index==0?c->wegert.half_height<6.0:c->wegert.half_height>0.015);
        button(u,zoom_ids[index],zoom_labels[index],(Rect){left,y,right-left,height},enabled);
    }
    y=u->content;
    for(int index=0;index<4;++index) {
        int left=margin+index*(width+gap)/4;
        int right=margin+(index+1)*(width+gap)/4-gap;
        bool enabled=index==0?c->wegert.center_real>-8.0:
                     index==3?c->wegert.center_real<8.0:
                     index==1?c->wegert.center_imag<8.0:c->wegert.center_imag>-8.0;
        button(u,pan_ids[index],pan_labels[index],(Rect){left,y,right-left,height},enabled);
    }
}

static void changed_shape(Console *c,Controls *u,const Partition *p)
{
    char *text=c->fields[SET_LAMBDA]; size_t used=0;
    text[0]=0;
    for(int row=0;row<p->count;++row)
        used+=(size_t)snprintf(text+used,512-used,"%s%d",row?",":"",p->rows[row]);
    if(!p->count) snprintf(text,512,"[]");
    c->scripted_facts[0]=0;
    console_run(c,DisplayPartition);
    console_run(c,ValidateTableau);
    u->focus=0;
}

bool nearby_event(Console *c,Controls *u,ControlEvent event)
{
    if(event.kind!=EVENT_ACTIVATE) return false;
    int id=event.id;
    bool minus=id>=ROW_MINUS_BASE && id<ROW_MINUS_BASE+YT_DIM;
    bool plus=id>=ROW_PLUS_BASE && id<ROW_PLUS_BASE+YT_DIM;
    if(minus || plus || id==ADD_ROW) {
        Partition p;
        if(partition_parse(c->fields[SET_LAMBDA],&p)!=YT_OK) return true;
        if(id==ADD_ROW) {
            if(p.count>=YT_DIM || partition_size(&p)>=YT_CELLS) return true;
            p.rows[p.count++]=1;
        } else {
            int row=id-(minus?ROW_MINUS_BASE:ROW_PLUS_BASE), change=minus?-1:1;
            if(!row_change_allowed(&p,row,change)) return true;
            p.rows[row]+=change;
            if(p.rows[row]==0) --p.count;
        }
        changed_shape(c,u,&p);
        return true;
    }
    if(id<PLOT_LEFT || id>PLOT_RESET) return false;
    double step=0.3*c->wegert.half_height;
    switch(id) {
    case PLOT_LEFT: c->wegert.center_real=fmax(-8.0,c->wegert.center_real-step); break;
    case PLOT_RIGHT: c->wegert.center_real=fmin(8.0,c->wegert.center_real+step); break;
    case PLOT_UP: c->wegert.center_imag=fmin(8.0,c->wegert.center_imag+step); break;
    case PLOT_DOWN: c->wegert.center_imag=fmax(-8.0,c->wegert.center_imag-step); break;
    case PLOT_ZOOM_IN: c->wegert.half_height=fmax(0.015,c->wegert.half_height/1.5); break;
    case PLOT_ZOOM_OUT: c->wegert.half_height=fmin(6.0,c->wegert.half_height*1.5); break;
    case PLOT_RESET:
        c->wegert.center_real=0.0; c->wegert.center_imag=0.0; c->wegert.half_height=1.5;
        break;
    default: break;
    }
    return true;
}
