#include "renderer/framebuffer.hpp"
#include <fstream>
#include <stdexcept>
namespace er {
std::string utf8(char32_t c) {
    std::string s;
    if(c<0x80) s+=char(c);
    else if(c<0x800) { s+=char(0xc0|(c>>6)); s+=char(0x80|(c&63)); }
    else if(c<0x10000) { s+=char(0xe0|(c>>12)); s+=char(0x80|((c>>6)&63)); s+=char(0x80|(c&63)); }
    else { s+=char(0xf0|(c>>18)); s+=char(0x80|((c>>12)&63)); s+=char(0x80|((c>>6)&63)); s+=char(0x80|(c&63)); } return s;
}
void Framebuffer::resize(int w,int h) { width=clamp(w,1,500); height=clamp(h,1,180); cells.assign(size_t(width*height),{}); previous.clear(); }
void Framebuffer::clear() { std::fill(cells.begin(),cells.end(),Cell{}); }
void Framebuffer::put(int x,int y,char32_t g,Color fg,Color bg) { if(x>=0&&y>=0&&x<width&&y<height) cells[size_t(y*width+x)]={g,fg,bg}; }
void Framebuffer::text(int x,int y,const std::string& s,Color fg) {
    for(size_t i=0;i<s.size();) {
        auto c=static_cast<unsigned char>(s[i++]); char32_t u=c; int rest=0;
        if((c&0xe0)==0xc0) { u=c&31; rest=1; } else if((c&0xf0)==0xe0) { u=c&15; rest=2; } else if((c&0xf8)==0xf0) { u=c&7; rest=3; }
        while(rest--&&i<s.size()) u=(u<<6)|(static_cast<unsigned char>(s[i++])&63);
        put(x++,y,u,fg);
    }
}
std::string Framebuffer::diff(bool force) {
    force=force||previous.size()!=cells.size(); std::string out; out.reserve(cells.size()*15); Color fg{},bg{}; bool color=false;
    for(int y=0;y<height;++y) {
        bool located=false;
        for(int x=0;x<width;++x) {
            const size_t i=size_t(y*width+x); const Cell& c=cells[i];
            if(!force&&c==previous[i]) { located=false; continue; }
            if(!located) { out+="\x1b["+std::to_string(y+1)+";"+std::to_string(x+1)+"H"; located=true; }
            if(!color||fg!=c.fg) { fg=c.fg; out+="\x1b[38;2;"+std::to_string(fg.r)+";"+std::to_string(fg.g)+";"+std::to_string(fg.b)+"m"; }
            if(!color||bg!=c.bg) { bg=c.bg; out+="\x1b[48;2;"+std::to_string(bg.r)+";"+std::to_string(bg.g)+";"+std::to_string(bg.b)+"m"; }
            color=true; out+=utf8(c.glyph);
        }
    } previous=cells; return out;
}
void Framebuffer::dump(const std::string& path) const {
    std::ofstream f(path); if(!f) throw std::runtime_error("Cannot write frame: "+path);
    f<<"{\"width\":"<<width<<",\"height\":"<<height<<",\"cells\":[";
    for(size_t i=0;i<cells.size();++i) {
        if(i) f<<',';
        char32_t c=cells[i].glyph; std::string s=utf8(c); if(c==U'\\'||c==U'\"') s="\\"+s;
        f<<"[\""<<s<<"\","<<int(cells[i].fg.r)<<','<<int(cells[i].fg.g)<<','<<int(cells[i].fg.b)<<']';
    } f<<"]}\n";
}
}
