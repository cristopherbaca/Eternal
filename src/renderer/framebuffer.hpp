#pragma once
#include "core/math.hpp"
#include <string>
#include <vector>
namespace er {
struct Cell {
    char32_t glyph=U' '; Color fg=white,bg=background;
    bool operator==(const Cell& b) const { return glyph==b.glyph&&fg==b.fg&&bg==b.bg; }
};
std::string utf8(char32_t c);
class Framebuffer {
public:
    int width=0,height=0; std::vector<Cell> cells;
    Framebuffer(int w=200,int h=60) { resize(w,h); }
    void resize(int w,int h); void clear();
    void put(int x,int y,char32_t glyph,Color fg=white,Color bg=background);
    void text(int x,int y,const std::string& text,Color fg=white);
    const Cell& at(int x,int y) const { return cells[size_t(y*width+x)]; }
    std::string diff(bool force=false); void dump(const std::string& path) const;
private: std::vector<Cell> previous;
};
}
