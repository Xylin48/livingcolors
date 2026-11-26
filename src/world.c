#include "world.h"
#include <math.h>
bool lc_add_box(LCWorld *w, LCBox a) {
 if (!w || w->count>=128 || !(a.min.x<a.max.x && a.min.y<a.max.y && a.min.z<a.max.z) || !isfinite(a.min.x)||!isfinite(a.min.y)||!isfinite(a.min.z)||!isfinite(a.max.x)||!isfinite(a.max.y)||!isfinite(a.max.z)) return false;
 w->solids[w->count++]=a; return true;
}
bool lc_blocked(const LCWorld *w, LCVec p) {
 if (!w || !isfinite(p.x)||!isfinite(p.y)||!isfinite(p.z)) return true;
 for(size_t i=0;i<w->count;i++){
 LCBox a=w->solids[i];
 if(p.x+.25f>a.min.x && p.x-.25f<a.max.x && p.y+1.5f>a.min.y && p.y<a.max.y && p.z+.25f>a.min.z && p.z-.25f<a.max.z) return true;
 } return false;
}
bool lc_move(const LCWorld *w,LCPose *p,float dx,float dz,float dt){
 if(!w||!p||!isfinite(dx)||!isfinite(dz)||!isfinite(dt)||dt<0||dt>1||lc_blocked(w,p->position)) return false;
 float len=hypotf(dx,dz); if(!isfinite(len)) return false;
 if(len>1){dx/=len;dz/=len;}
 int steps=(int)ceilf(dt*40); if(!steps)return true;
 for(int i=0;i<steps;i++){
 LCVec q=p->position;q.x+=dx*2*dt/steps;if(!lc_blocked(w,q))p->position=q;
 q=p->position;q.z+=dz*2*dt/steps;if(!lc_blocked(w,q))p->position=q;
 } return true;
}
static void box(LCWorld*w,float x,float y,float z,float X,float Y,float Z){
 lc_add_box(w,(LCBox){{x,y,z},{X,Y,Z}});
}
void lc_house(LCWorld*w){
 if(!w)return;
 *w=(LCWorld){0};
 /* Provisional metric ground-floor geometry, front at positive Z. */
 box(w,0,0,0,.15f,2.7f,10);
 box(w,11.85f,0,0,12,2.7f,10);
 box(w,0,0,0,6.2f,2.7f,.15f);
 box(w,7.2f,0,0,12,2.7f,.15f);
 box(w,0,0,9.85f,5.5f,2.7f,10);
 box(w,6.5f,0,9.85f,12,2.7f,10);
 box(w,0,0,4.2f,8.5f,2.7f,4.35f);
 box(w,10,0,4.2f,12,2.7f,4.35f);
}
