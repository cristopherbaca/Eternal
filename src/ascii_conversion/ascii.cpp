#include "ascii_conversion/ascii.hpp"
namespace er {
float luminance(Color c) { return (.2126f*c.r+.7152f*c.g+.0722f*c.b)/255; }
char32_t brightnessGlyph(float value) { return U" .:-=+*#%@"[clamp(int(value*9+.5f),0,9)]; }
char32_t edgeGlyph(float dx,float dy) {
    if(std::abs(dx)>std::abs(dy)*2) return U'|';
    if(std::abs(dy)>std::abs(dx)*2) return U'-';
    return dx*dy>0?U'/':U'\\';
}
Color shade(const Pixel& p,const std::vector<Light>& lights,float vignette) {
    float diffuse=clamp(-p.normal.x*.35f-p.normal.y*.25f+p.normal.z*.8f,0.f,1.f);
    float r=p.color.r*(.48f+.52f*diffuse),g=p.color.g*(.48f+.52f*diffuse),b=p.color.b*(.55f+.45f*diffuse);
    for(const auto& l:lights) {
        float d=std::sqrt(dot(p.world-l.pos,p.world-l.pos)+(p.height-l.height)*(p.height-l.height)*.7f);
        float falloff=std::max(0.f,1-d/l.radius); falloff*=falloff*l.intensity;
        r+=l.color.r*falloff*.85f; g+=l.color.g*falloff*.75f; b+=l.color.b*falloff*.65f;
    }
    if(p.emission>0) { r=std::max(r,float(p.color.r)); g=std::max(g,float(p.color.g)); b=std::max(b,float(p.color.b)); }
    return rgb(r*vignette,g*vignette,b*vignette);
}
void Scene::convert(Framebuffer& f) const {
    for(int y=0;y<camera.height;++y) for(int x=0;x<camera.width;++x) {
        size_t i=size_t(y*camera.width+x); const Pixel& p=pixels[i];
        float nx=(x-camera.width*.5f)/(camera.width*.6f),ny=(y-camera.height*.5f)/(camera.height*.8f);
        float vignette=clamp(1.08f-.42f*(nx*nx+ny*ny),.38f,1.f);
        Color col=shade(p,lights,vignette); char32_t glyph=p.shape;
        if(!glyph) {
            glyph=brightnessGlyph(luminance(col)*1.8f);
            if(x>0&&y>0&&x+1<camera.width&&y+1<camera.height) {
                float dx=pixels[i+1].height-pixels[i-1].height,dy=pixels[i+size_t(camera.width)].height-pixels[i-size_t(camera.width)].height;
                if(dx*dx+dy*dy>4) glyph=edgeGlyph(dx,dy);
            }
        }
        f.cells[i]={glyph,col,background};
    }
}
}
