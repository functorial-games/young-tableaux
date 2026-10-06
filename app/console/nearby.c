#include "nearby.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>

static void button(Controls *u,int id,const char *label,Rect rect,bool enabled)
{
    controls_add_at(u,id,enabled?BUTTON:DISABLED_BUTTON,label,rect,NULL);
}

static bool same_partition(const Partition *a,const Partition *b)
{
    if(a->count!=b->count) return false;
    for(int row=0;row<a->count;++row) if(a->rows[row]!=b->rows[row]) return false;
    return true;
}

static void sync_history(Console *c,const Partition *current)
{
    if(same_partition(&c->diagram.current,current)) return;
    diagram_replace(&c->diagram,current);
}

void nearby_shape(Console *c,Controls *u)
{
    Partition p;
    if(partition_parse(c->fields[SET_LAMBDA],&p)!=YT_OK) return;
    sync_history(c,&p);

    Cell cells[YT_DIM+1];
    bool addable[YT_DIM+1]={false};
    bool removable[YT_DIM]={false};
    int addable_count=partition_addable(&p,cells);
    for(int i=0;i<addable_count;++i) {
        int row=cells[i].row-1;
        if(row>=0 && row<YT_DIM && cells[i].column<=YT_DIM) addable[row]=true;
    }
    int removable_count=partition_removable(&p,cells);
    for(int i=0;i<removable_count;++i) {
        int row=cells[i].row-1;
        if(row>=0 && row<YT_DIM) removable[row]=true;
    }

    int scale=u->scale, margin=4*scale, gap=4*scale, side=24*scale;
    int width=u->width-2*margin;
    int columns=(p.count?p.rows[0]:0)+1;
    int available=width-4*scale;
    int cell=side;
    if(columns*cell>available) cell=available/columns;
    if(cell<1) cell=1;

    bool can_new_row=p.count<YT_DIM && partition_size(&p)<YT_CELLS && addable[p.count];
    int visual_rows=p.count+(can_new_row?1:0);
    if(!visual_rows) visual_rows=1;

    for(int visual=0;visual<visual_rows;++visual) {
        int row;
        if(can_new_row) row=c->french?p.count-visual:visual;
        else row=c->french?p.count-1-visual:visual;
        int y=u->content;

        if(row>=0 && row<p.count) {
            c->editable_rows[row]=(TileRowProjection){p.rows[row],columns};
            controls_add_at(u,0,ROW_BLOCKS,"",(Rect){margin,y,width,side},&c->editable_rows[row]);

            int button_side=cell;
            if(button_side>side-4*scale) button_side=side-4*scale;
            if(button_side<1) button_side=1;

            if(removable[row]) {
                int x=margin+2*scale+(p.rows[row]-1)*cell;
                if(x+button_side>u->width-margin) x=u->width-margin-button_side;
                button(u,REMOVABLE_BASE+row,"−",
                       (Rect){x,y+(side-button_side)/2,button_side,button_side},true);
            }
            if(addable[row] && partition_size(&p)<YT_CELLS) {
                int x=margin+2*scale+p.rows[row]*cell;
                if(x+button_side>u->width-margin) x=u->width-margin-button_side;
                button(u,ADDABLE_BASE+row,"+",
                       (Rect){x,y+(side-button_side)/2,button_side,button_side},true);
            }
        } else if(row==p.count && can_new_row) {
            int button_side=cell;
            if(button_side>side-4*scale) button_side=side-4*scale;
            if(button_side<1) button_side=1;
            button(u,ADDABLE_BASE+row,"+",
                   (Rect){margin+2*scale,y+(side-button_side)/2,button_side,button_side},true);
        } else {
            controls_add_at(u,0,ROW_BLOCKS,"",(Rect){margin,y,width,side},NULL);
        }
        u->content=y+side;
    }

    u->content+=gap;
    char stats[96];
    uint64_t dimension=0;
    MathStatus dimension_status=partition_standard_count(&p,&dimension);
    if(dimension_status==YT_OK)
        snprintf(stats,sizeof(stats),"|λ| = %d\nstandard tableaux = %" PRIu64,partition_size(&p),dimension);
    else
        snprintf(stats,sizeof(stats),"|λ| = %d\nstandard tableaux = %s",partition_size(&p),math_status(dimension_status));
    int stats_x=margin+width/3, stats_width=width-stats_x+margin;
    int stats_height=controls_text_height(u,stats);
    controls_add_at(u,0,LABEL,stats,(Rect){stats_x,u->content,stats_width,stats_height},NULL);

    int y=u->content, half=(width-gap)/2;
    bool can_rewind=c->diagram.addition_count>0;
    button(u,SHAPE_UNDO,"Undo",(Rect){margin,y,half,side},can_rewind);
    button(u,SHAPE_RESET,"Reset",(Rect){margin+half+gap,y,width-half-gap,side},can_rewind);
}

void nearby_plot_controls(Console *c,Controls *u)
{
    const int zoom_ids[]={PLOT_ZOOM_OUT,PLOT_RESET,PLOT_ZOOM_IN};
    const char *zoom_labels[]={"Zoom −","Reset","Zoom +"};
    const int pan_ids[]={PLOT_LEFT,PLOT_UP,PLOT_DOWN,PLOT_RIGHT};
    const char *pan_labels[]={"Left","Up","Down","Right"};
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

static bool find_addable(const Partition *p,int row,Cell *out)
{
    if(row<0 || row>p->count || row>=YT_DIM || partition_size(p)>=YT_CELLS) return false;
    Cell cells[YT_DIM+1];
    int count=partition_addable(p,cells);
    for(int i=0;i<count;++i) if(cells[i].row==row+1 && cells[i].column<=YT_DIM) {
        *out=cells[i];
        return true;
    }
    return false;
}

static bool find_removable(const Partition *p,int row,Cell *out)
{
    if(row<0 || row>=p->count || row>=YT_DIM) return false;
    Cell cells[YT_DIM];
    int count=partition_removable(p,cells);
    for(int i=0;i<count;++i) if(cells[i].row==row+1) {
        *out=cells[i];
        return true;
    }
    return false;
}

bool nearby_event(Console *c,Controls *u,ControlEvent event)
{
    if(event.kind!=EVENT_ACTIVATE) return false;
    int id=event.id;

    if(id>=REMOVABLE_BASE && id<REMOVABLE_BASE+YT_DIM) {
        Partition p;
        if(partition_parse(c->fields[SET_LAMBDA],&p)!=YT_OK) return true;
        int row=id-REMOVABLE_BASE;
        Cell cell;
        if(!find_removable(&p,row,&cell)) return true;
        sync_history(c,&p);
        if(diagram_remove_cell(&c->diagram,cell)!=YT_OK) return true;
        changed_shape(c,u,&c->diagram.current);
        return true;
    }

    if(id>=ADDABLE_BASE && id<=ADDABLE_BASE+YT_DIM) {
        Partition p;
        if(partition_parse(c->fields[SET_LAMBDA],&p)!=YT_OK) return true;
        sync_history(c,&p);
        int row=id-ADDABLE_BASE;
        Cell cell;
        if(!find_addable(&p,row,&cell)) return true;
        if(diagram_add_cell(&c->diagram,cell)!=YT_OK) return true;
        changed_shape(c,u,&c->diagram.current);
        return true;
    }

    if(id==SHAPE_UNDO || id==SHAPE_RESET) {
        Partition p;
        if(partition_parse(c->fields[SET_LAMBDA],&p)!=YT_OK) return true;
        sync_history(c,&p);
        if(!c->diagram.addition_count) return true;
        if(id==SHAPE_RESET) {
            diagram_reset_additions(&c->diagram);
        } else {
            if(diagram_undo_addition(&c->diagram)!=YT_OK) return true;
        }
        changed_shape(c,u,&c->diagram.current);
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
