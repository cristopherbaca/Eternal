#pragma once
#include "renderer/scene.hpp"
namespace er {
inline void drawDragon(Scene& s,Vec2 pos,float time) {
    Vec2 base=s.camera.project(pos); float zoom=s.camera.scale(),flap=std::sin(time*2.8f);
    for(int yy=int(base.y-28*zoom);yy<=int(base.y+4*zoom);++yy) for(int xx=int(base.x-39*zoom);xx<=int(base.x+39*zoom);++xx) {
        float x=(xx-base.x)/(2.2f*zoom),h=(base.y-yy)/(1.5f*zoom),a=std::abs(x);
        bool body=a<2.2f&&h>1&&h<11,head=((a<1.6f&&h>=11&&h<14)||(x>0&&x<4.5f&&h>10.7f&&h<12.4f));
        float upper=8+(a-2)*(.48f+flap*.13f),lower=3+(a-2)*.36f+std::abs(std::sin(a*.5f))*1.5f;
        bool wing=a>1.8f&&a<17&&h>lower&&h<upper,tail=std::abs(x-std::sin(h)*2)<.7f&&h<2&&h>-2,legs=a>1&&a<5&&h>0&&h<2.2f-a*.22f;
        if(!(body||head||wing||tail||legs)) continue;
        float n=noise(int(x*4),int(h*4),19); Color col=mix({66,29,35},{168,91,66},n); char32_t glyph=U"oO0Q@%#"[int(n*6.99f)];
        if(wing) { float membrane=(h-lower)/std::max(.1f,upper-lower); col=mix({33,22,34},{131,65,57},membrane); glyph=membrane>.8f?(x<0?U'/':U'\\'):U".:=o"[int(n*3.99f)]; }
        if(head&&x>1&&h>12) { col={245,164,66}; glyph=U'*'; }
        s.put(xx,yy,pos,std::max(0.f,h),col,glyph,pos.y+.3f,{x*.02f,-.3f,.8f});
    }
}
}
