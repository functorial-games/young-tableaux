#include <android/input.h>
#include <android/native_window.h>
#include <android_native_app_glue.h>
#include "console.h"
#include "paint.h"
#include <stdlib.h>
#include <string.h>
typedef struct {
    struct android_app *app; Console console; Controls ui;
    int width,height,key_pointer,key_down; bool focused,dirty;
} App;
static void draw(App *state)
{
    if(!state->app->window) return;
    ANativeWindow_setBuffersGeometry(state->app->window,0,0,WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_Buffer buffer;
    if(ANativeWindow_lock(state->app->window,&buffer,NULL)!=0) return;
    state->width=buffer.width; state->height=buffer.height;
    console_layout(&state->console,&state->ui,buffer.width,buffer.height);
    Canvas canvas={buffer.width,buffer.height,buffer.stride,buffer.bits,0,buffer.height};
    paint_controls(&canvas,&state->ui,state->console.french);
    if(state->ui.focus) {
        const char *labels[20]; for(int k=0;k<20;++k) labels[k]=console_key_label(k);
        paint_keyboard(&canvas,&state->ui,labels,state->key_down);
    }
    ANativeWindow_unlockAndPost(state->app->window); state->dirty=false;
}
static void cancel(App *state)
{
    controls_touch(&state->ui,TOUCH_CANCEL,0,0,0);
    state->key_pointer=-1; state->key_down=-1;
}
static void command(struct android_app *app,int32_t cmd)
{
    App *s=app->userData;
    switch(cmd) {
    case APP_CMD_GAINED_FOCUS: case APP_CMD_RESUME: s->focused=true; s->dirty=true; break;
    case APP_CMD_INIT_WINDOW: case APP_CMD_WINDOW_RESIZED: case APP_CMD_CONFIG_CHANGED:
    case APP_CMD_CONTENT_RECT_CHANGED: s->dirty=true; cancel(s); break;
    case APP_CMD_LOST_FOCUS: case APP_CMD_PAUSE: case APP_CMD_TERM_WINDOW: s->focused=false; cancel(s); break;
    case APP_CMD_SAVE_STATE: {
        /* Only application data, never pointers or drawn rectangles. */
        void *saved=malloc(sizeof(Console));
        if(saved) { memcpy(saved,&s->console,sizeof(Console)); app->savedState=saved; app->savedStateSize=sizeof(Console); }
        break;
    }
    default: break;
    }
}
static int32_t input(struct android_app *app,AInputEvent *event)
{
    App *s=app->userData;
    if(AInputEvent_getType(event)==AINPUT_EVENT_TYPE_KEY) {
        if(AKeyEvent_getKeyCode(event)==AKEYCODE_BACK && s->ui.focus) {
            if(AKeyEvent_getAction(event)==AKEY_EVENT_ACTION_UP) { s->ui.focus=0; cancel(s); s->dirty=true; }
            return 1;
        }
        return 0;
    }
    if(AInputEvent_getType(event)!=AINPUT_EVENT_TYPE_MOTION || !s->app->window || !s->focused) return 0;
    int action=AMotionEvent_getAction(event), masked=action&AMOTION_EVENT_ACTION_MASK;
    if(masked==AMOTION_EVENT_ACTION_CANCEL) { cancel(s); s->dirty=true; return 1; }
    int count=(int)AMotionEvent_getPointerCount(event);
    int index=(action&AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)>>AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
    if(masked==AMOTION_EVENT_ACTION_MOVE) {
        int active=s->key_pointer>=0?s->key_pointer:s->ui.pointer;
        index=-1;
        for(int i=0;i<count;++i) if(AMotionEvent_getPointerId(event,(size_t)i)==active) index=i;
        if(index<0) return 1;
    }
    if(index<0 || index>=count) return 1;
    int pointer=AMotionEvent_getPointerId(event,(size_t)index);
    /* NativeActivity motion and native-window pixels share this coordinate space. */
    int x=(int)AMotionEvent_getX(event,(size_t)index),y=(int)AMotionEvent_getY(event,(size_t)index);
    Touch touch;
    if(masked==AMOTION_EVENT_ACTION_DOWN || masked==AMOTION_EVENT_ACTION_POINTER_DOWN) touch=TOUCH_DOWN;
    else if(masked==AMOTION_EVENT_ACTION_UP || masked==AMOTION_EVENT_ACTION_POINTER_UP) touch=TOUCH_UP;
    else if(masked==AMOTION_EVENT_ACTION_MOVE) touch=TOUCH_MOVE;
    else return 1;
    if(touch==TOUCH_DOWN && (s->ui.pointer>=0 || s->key_pointer>=0)) return 1;
    int key=s->ui.focus?console_key_hit(&s->ui,x,y,s->height):-1;
    if(touch==TOUCH_DOWN && key>=0) { s->key_pointer=pointer; s->key_down=key; }
    else if(s->key_pointer==pointer) {
        if(touch==TOUCH_MOVE && key!=s->key_down) s->key_down=-1;
        if(touch==TOUCH_UP) { if(key>=0 && key==s->key_down) console_key(&s->console,&s->ui,key); s->key_pointer=-1; s->key_down=-1; }
    } else {
        ControlEvent semantic=controls_touch(&s->ui,touch,pointer,x,y);
        console_event(&s->console,&s->ui,semantic);
        if(semantic.kind==EVENT_FOCUS) {
            console_layout(&s->console,&s->ui,s->width,s->height);
            for(int i=0;i<s->ui.count;++i) if(s->ui.controls[i].id==s->ui.focus) {
                Rect r=s->ui.controls[i].rect;
                if(r.y+r.h-s->ui.scroll>s->ui.height) controls_scroll(&s->ui,r.y+r.h-s->ui.height);
                if(r.y<s->ui.scroll) controls_scroll(&s->ui,r.y);
            }
        }
    }
    s->dirty=true; return 1;
}
void android_main(struct android_app *app)
{
    App *s=calloc(1,sizeof(*s)); if(!s) return;
    s->app=app; console_init(&s->console); controls_init(&s->ui);
    s->key_pointer=-1; s->key_down=-1; s->dirty=true;
    if(app->savedState && app->savedStateSize==sizeof(Console)) memcpy(&s->console,app->savedState,sizeof(Console));
    app->userData=s; app->onAppCmd=command; app->onInputEvent=input;
    while(!app->destroyRequested) {
        int events; struct android_poll_source *source=NULL;
        ALooper_pollOnce(s->dirty?0:-1,NULL,&events,(void **)&source);
        if(source) source->process(app,source);
        if(app->destroyRequested) break;
        if(s->dirty && app->window) draw(s);
        /* If no window exists, block for the next lifecycle event. */
        else s->dirty=false;
    }
    raster_destroy(); free(s); app->userData=NULL;
}
