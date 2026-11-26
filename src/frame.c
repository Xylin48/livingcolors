#include "frame.h"
#include "scene.h"
#include "body.h"
#include <math.h>

#define PI 3.14159265358979323846f
#define NEAR .01f
typedef struct { float x,y,invz; } Screen;
typedef struct {
    const LCWorld *world;
    LCView view;
    float focal;
    uint8_t *rgb;
    float *depth;
} Raster;
static float dot(LCVec a,LCVec b){return a.x*b.x+a.y*b.y+a.z*b.z;}
static LCVec camera_point(const Raster *r,LCVec p){
    LCVec d={p.x-r->view.origin.x,p.y-r->view.origin.y,p.z-r->view.origin.z};
    return (LCVec){dot(d,r->view.right),dot(d,r->view.up),dot(d,r->view.forward)};
}
static Screen project(const Raster *r,LCVec p){
    float iz=1/p.z;
    return (Screen){LC_VIEW_WIDTH*.5f+r->focal*p.x*iz,LC_VIEW_HEIGHT*.5f-r->focal*p.y*iz,iz};
}
static float edge(Screen a,Screen b,float x,float y){
    return (b.x-a.x)*(y-a.y)-(b.y-a.y)*(x-a.x);
}
static void triangle(Raster *r,Screen a,Screen b,Screen c,LCColor color){
    float area=edge(a,b,c.x,c.y);
    if(!isfinite(area)||fabsf(area)<.00001f)return;
    if(area<0){Screen t=b;b=c;c=t;area=-area;}
    float xmin=fmaxf(0,fminf(a.x,fminf(b.x,c.x)));
    float xmax=fminf(LC_VIEW_WIDTH-1,fmaxf(a.x,fmaxf(b.x,c.x)));
    float ymin=fmaxf(0,fminf(a.y,fminf(b.y,c.y)));
    float ymax=fminf(LC_VIEW_HEIGHT-1,fmaxf(a.y,fmaxf(b.y,c.y)));
    if(xmin>xmax||ymin>ymax)return;
    int x0=(int)floorf(xmin),x1=(int)ceilf(xmax);
    int y0=(int)floorf(ymin),y1=(int)ceilf(ymax);
    float invarea=1/area;
    for(int y=y0;y<=y1;y++){
        for(int x=x0;x<=x1;x++){
            float px=x+.5f,py=y+.5f;
            float wa=edge(b,c,px,py),wb=edge(c,a,px,py),wc=edge(a,b,px,py);
            if(wa<0||wb<0||wc<0)continue;
            float iz=(wa*a.invz+wb*b.invz+wc*c.invz)*invarea;
            size_t index=(size_t)y*LC_VIEW_WIDTH+(size_t)x;
            if(iz<=r->depth[index])continue;
            r->depth[index]=iz;
            r->rgb[index*3]=color.r;r->rgb[index*3+1]=color.g;r->rgb[index*3+2]=color.b;
        }
    }
}
/* Clip each convex face against the camera near plane before projection.
 * A clipped quad can have at most five vertices. */
static void face(Raster *r,const LCVec in[4],LCColor color){
    LCVec clipped[8];
    size_t n=0;
    for(size_t i=0;i<4;i++){
        LCVec a=in[i],b=in[(i+1)%4];
        bool inside=a.z>=NEAR,next=b.z>=NEAR;
        if(inside)clipped[n++]=a;
        if(inside!=next){
            float t=(NEAR-a.z)/(b.z-a.z);
            clipped[n++]=(LCVec){a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,NEAR};
        }
    }
    for(size_t i=1;i+1<n;i++)
        triangle(r,project(r,clipped[0]),project(r,clipped[i]),project(r,clipped[i+1]),color);
}
static bool finite_vec(LCVec p){return isfinite(p.x)&&isfinite(p.y)&&isfinite(p.z);}
static void draw_part(const LCScenePart *p,void *ctx){
    Raster *r=ctx;
    LCBox b=p->bounds;
    if(!finite_vec(b.min)||!finite_vec(b.max)||b.min.x>=b.max.x||b.min.y>=b.max.y||b.min.z>=b.max.z)return;
    LCVec v[8];
    for(unsigned i=0;i<8;i++){
        v[i]=camera_point(r,(LCVec){i&1?b.max.x:b.min.x,i&2?b.max.y:b.min.y,i&4?b.max.z:b.min.z});
        if(!finite_vec(v[i]))return;
    }
    const unsigned faces[6][4]={{0,2,6,4},{1,5,7,3},{0,4,5,1},{2,3,7,6},{0,1,3,2},{4,6,7,5}};
    /* Cull faces looking away from the eye. Interior surfaces are included
     * if a near-plane crossing leaves the eye on the box's boundary. */
    LCVec e=r->view.origin;
    bool visible[6]={e.x<=b.min.x,e.x>=b.max.x,e.y<=b.min.y,e.y>=b.max.y,e.z<=b.min.z,e.z>=b.max.z};
    LCColor lit=lc_scene_lit_color(r->world,p);
    for(size_t i=0;i<6;i++){
        if(!visible[i])continue;
        LCVec poly[4]={v[faces[i][0]],v[faces[i][1]],v[faces[i][2]],v[faces[i][3]]};
        face(r,poly,lit);
    }
}
bool lc_frame_render(const LCWorld *w,uint8_t *rgb,size_t bytes,float *depth,size_t count){
    if(!rgb||!depth||bytes<LC_FRAME_RGB_BYTES||count<LC_FRAME_PIXELS||!lc_body_valid(w))return false;
    Raster r={.world=w,.rgb=rgb,.depth=depth};
    if(!lc_world_view(w,&r.view))return false;
    r.focal=LC_VIEW_HEIGHT*.5f/tanf(r.view.vertical_fov_degrees*PI/360);
    LCColor sky=lc_scene_sky(w);
    for(size_t i=0;i<LC_FRAME_PIXELS;i++){
        depth[i]=0;rgb[i*3]=sky.r;rgb[i*3+1]=sky.g;rgb[i*3+2]=sky.b;
    }
    return lc_scene_visit(w,draw_part,&r);
}
