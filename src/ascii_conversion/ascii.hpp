#pragma once
#include "renderer/scene.hpp"
namespace er {
float luminance(Color c);
char32_t brightnessGlyph(float value);
char32_t edgeGlyph(float dx,float dy);
Color shade(const Pixel& p,const std::vector<Light>& lights,float vignette);
}
