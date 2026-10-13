#include "platform/window.hpp"
#include "platform/mac_runtime.hpp"
#include <array>
#include <cstdlib>
#include <stdexcept>
using namespace er::mac;
namespace er {
struct Window::Impl {
    Object window;
    Object layer;
    Object device;
    Object queue;
    Object pipeline,terrainPipeline;
    Object depthState;
    Object asciiPipeline;
    Object terrainCells;
    Object color3D,normal3D,depth3D;
    Object meshBuffer;
    size_t meshVertices=0; uint64_t meshRevision=~uint64_t(0);
    bool raw=false,debug=false,mouseCaptured=false;
    Object atlas;
    Object cells,interface;
    Object pending,pendingTerrain;
    std::array<bool,128> held{};
    int cols=320,rows=100,uiCols=160,uiRows=50;
};
static const char* shader=R"(
#include <metal_stdlib>
using namespace metal;
struct Cell { uint glyph; uint fg; uint bg; float depth; };
struct Vertex { float4 position [[position]]; float2 uv; };
vertex Vertex vertices(uint id [[vertex_id]]) {
    float2 p=float2((id==1)?3.0:-1.0,(id==2)?3.0:-1.0);
    return {float4(p,0,1),float2((p.x+1)*0.5,(1-p.y)*0.5)};
}
float3 rgb(uint v) { return float3(v&255,(v>>8)&255,(v>>16)&255)/255.0; }
struct MeshVertex { float4 position; float4 normal; float4 color; };
struct View3D { float4 eye,right,up,forward,lens; };
struct TerrainOut { float4 position [[position]]; float3 normal; float3 color; float3 world; };
vertex TerrainOut terrainVertex(uint id [[vertex_id]],const device MeshVertex* mesh [[buffer(0)]],constant View3D& view [[buffer(1)]]) {
    MeshVertex v=mesh[id]; float3 d=v.position.xyz-view.eye.xyz;
    float z=dot(d,view.forward.xyz),near=view.lens.z,far=view.lens.w;
    float4 clip=float4(dot(d,view.right.xyz)*view.lens.x/view.lens.y,dot(d,view.up.xyz)*view.lens.x,far/(far-near)*z-far*near/(far-near),z);
    return {clip,v.normal.xyz,v.color.xyz,v.position.xyz};
}
struct TerrainTargets { float4 color [[color(0)]]; float4 normal [[color(1)]]; };
fragment TerrainTargets terrainFragment(TerrainOut in [[stage_in]]) {
    float3 n=normalize(in.normal);
    float light=.32+.68*max(0.0,dot(n,normalize(float3(-.5,-.35,.8))));
    return {float4(in.color*light,1),float4(n*.5+.5,1)};
}
// Convert once per source cell, rather than repeating edge detection per display pixel.
uint packColor(float3 c) {
    uint3 b=uint3(clamp(c*255.0,0.0,255.0));
    return b.x|(b.y<<8)|(b.z<<16);
}
kernel void asciiCells(texture2d<float,access::read> color [[texture(0)]],
                       texture2d<float,access::read> normals [[texture(1)]],
                       depth2d<float,access::read> depth [[texture(2)]],
                       device Cell* output [[buffer(0)]],constant View3D& view [[buffer(1)]],
                       uint2 cell [[thread_position_in_grid]]) {
    uint2 size=uint2(color.get_width(),color.get_height());
    if(any(cell>=size)) return;
    uint index=cell.y*size.x+cell.x;
    float d=depth.read(cell);
    float3 navy=float3(.043,.043,.102);
    if(d>=.999999) { output[index]={32,packColor(navy),packColor(navy),0}; return; }
    float3 source=color.read(cell).rgb;
    float3 n=normalize(normals.read(cell).xyz*2-1);
    // Reconstruct position to anchor detail to the terrain, never frame time or screen noise.
    float z=view.lens.z*view.lens.w/(view.lens.w-d*(view.lens.w-view.lens.z));
    float2 uv=(float2(cell)+.5)/float2(size);
    float3 world=view.eye.xyz+z*(view.forward.xyz+view.right.xyz*((uv.x*2-1)*view.lens.y/view.lens.x)+view.up.xyz*((1-uv.y*2)/view.lens.x));
    float grain=sin(world.x*1.7+sin(world.y*.9))*sin(world.y*1.3);
    // Fade sub-cell detail into the distance to avoid aliasing as the camera moves.
    grain*=1-smoothstep(12.0,55.0,z);
    float light=dot(source,float3(.2126,.7152,.0722));
    constexpr char ramp[]=" .,:;irsXA#@";
    uint glyph=uint(ramp[clamp(int(light*16+grain*.75),1,10)]);
    float2 gradient=float2(0); float strength=0;
    for(int y=-1;y<=1;++y) for(int x=-1;x<=1;++x) {
        if(x==0&&y==0) continue;
        uint2 q=uint2(clamp(int2(cell)+int2(x,y),int2(0),int2(size)-1));
        float neighbor=depth.read(q);
        float3 nn=normals.read(q).xyz*2-1;
        float nz=view.lens.z*view.lens.w/(view.lens.w-neighbor*(view.lens.w-view.lens.z));
        // Relative depth jumps identify silhouettes; normal changes identify creases.
        float edge=neighbor>=.999999?1.0:max(smoothstep(.18,.5,abs(nz-z)/max(z,1.0)),smoothstep(.16,.4,length(nn-n)));
        gradient+=float2(x,y)*edge; strength=max(strength,edge);
    }
    if(strength>.5&&length(gradient)>.4) {
        float2 a=abs(gradient);
        glyph=a.x>a.y*2?uint('|'):a.y>a.x*2?uint('_'):(gradient.x*gradient.y>0?uint('/'):uint('\\'));
    }
    // A faint source-colored cell background preserves mass and distant silhouettes.
    float3 foreground=clamp(source*1.45+float3(.055)+strength*.10,0.0,1.0);
    float3 background=mix(navy,source,.25);
    output[index]={glyph,packColor(foreground),packColor(background),0};
}
fragment float4 draw(Vertex in [[stage_in]], constant Cell* cells [[buffer(0)]],
                     constant uint2& grid [[buffer(1)]], texture2d<float> atlas [[texture(0)]],
                     constant Cell* interface [[buffer(2)]], constant uint2& uiGrid [[buffer(3)]],
                     texture2d<float> sceneColor [[texture(1)]],constant uint& mode [[buffer(4)]],constant float& backdrop [[buffer(5)]],constant Cell* terrainCells [[buffer(6)]],depth2d<float> terrainDepth [[texture(2)]]) {
    float2 p=clamp(in.uv,float2(0),float2(0.999999))*float2(grid);
    uint2 cell=uint2(p);
    Cell c=cells[cell.y*grid.x+cell.x];
    if(mode!=0&&c.depth>0&&c.depth>terrainDepth.read(cell)+.000002) c.glyph=32;
    bool worldVisible=c.glyph>32;
    float3 source=float3(0);
    if(mode!=0) {
        source=sceneColor.read(min(cell,uint2(sceneColor.get_width()-1,sceneColor.get_height()-1))).rgb*backdrop;
        if(c.glyph<=32) {
            c=terrainCells[cell.y*grid.x+cell.x];
            c.fg=packColor(rgb(c.fg)*backdrop); c.bg=packColor(rgb(c.bg)*backdrop);
        }
    }
    float2 ui=clamp(in.uv,float2(0),float2(0.999999))*float2(uiGrid);
    Cell overlay=interface[uint(ui.y)*uiGrid.x+uint(ui.x)];
    if(mode==2&&overlay.glyph==0&&!worldVisible) return float4(source,1);
    if(overlay.glyph!=0) { c=overlay; p=ui; }
    float2 tile=float2(c.glyph%16,c.glyph/16);
    float2 local=fract(p);
    constexpr sampler sample(filter::linear,address::clamp_to_edge);
    float a=atlas.sample(sample,(tile*float2(16,24)+float2(0.5)+local*float2(15,23))/float2(256,432)).r;
    return float4(mix(rgb(c.bg),rgb(c.fg),a),1);
}
)";
Window::Window():impl(std::make_unique<Impl>()) {
    { Pool pool;
        auto& v=*impl;
        registerClasses();
        send<id>(klass("NSApplication"),"sharedApplication");
        send<void>(NSApp,"setActivationPolicy:" ,NSApplicationActivationPolicyRegular);
        v.device=autoreleased(MTLCreateSystemDefaultDevice());
        if(!v.device) throw std::runtime_error("Metal GPU unavailable; run without --window");
        id  error=nullptr;
        id library=autoreleased(send<id>(v.device,"newLibraryWithSource:options:error:" ,send<id>(klass("NSString"),"stringWithUTF8String:" ,shader),nullptr,&error));
        if(!library) throw std::runtime_error(send<const char*>(send<id>(error,"localizedDescription"),"UTF8String"));
        v.asciiPipeline=autoreleased(send<id>(v.device,"newComputePipelineStateWithFunction:error:" ,autoreleased(send<id>(library,"newFunctionWithName:" ,str("asciiCells"))),&error));
        if(!v.asciiPipeline) throw std::runtime_error(send<const char*>(send<id>(error,"localizedDescription"),"UTF8String"));
        id  descriptor=autoreleased(send<id>(klass("MTLRenderPipelineDescriptor"),"new"));
        send<void>(descriptor,"setVertexFunction:",autoreleased(send<id>(library,"newFunctionWithName:" ,str("vertices"))));
        send<void>(descriptor,"setFragmentFunction:",autoreleased(send<id>(library,"newFunctionWithName:" ,str("draw"))));
        send<void>(at(send<id>(descriptor,"colorAttachments"),0),"setPixelFormat:",MTLPixelFormatBGRA8Unorm);
        v.pipeline=autoreleased(send<id>(v.device,"newRenderPipelineStateWithDescriptor:error:" ,descriptor,&error));
        if(!v.pipeline) throw std::runtime_error(send<const char*>(send<id>(error,"localizedDescription"),"UTF8String"));
        send<void>(descriptor,"setVertexFunction:",autoreleased(send<id>(library,"newFunctionWithName:" ,str("terrainVertex"))));
        send<void>(descriptor,"setFragmentFunction:",autoreleased(send<id>(library,"newFunctionWithName:" ,str("terrainFragment"))));
        send<void>(at(send<id>(descriptor,"colorAttachments"),0),"setPixelFormat:",MTLPixelFormatRGBA16Float);
        send<void>(at(send<id>(descriptor,"colorAttachments"),1),"setPixelFormat:",MTLPixelFormatRGBA16Float);
        send<void>(descriptor,"setDepthAttachmentPixelFormat:",MTLPixelFormatDepth32Float);
        v.terrainPipeline=autoreleased(send<id>(v.device,"newRenderPipelineStateWithDescriptor:error:" ,descriptor,&error));
        if(!v.terrainPipeline) throw std::runtime_error(send<const char*>(send<id>(error,"localizedDescription"),"UTF8String"));
        id  depth=autoreleased(send<id>(klass("MTLDepthStencilDescriptor"),"new")); send<void>(depth,"setDepthCompareFunction:",MTLCompareFunctionLess); send<void>(depth,"setDepthWriteEnabled:",true);
        v.depthState=autoreleased(send<id>(v.device,"newDepthStencilStateWithDescriptor:" ,depth));
        v.queue=autoreleased(send<id>(v.device,"newCommandQueue"));
        auto dummyDepth=send<id>(klass("MTLTextureDescriptor"),"texture2DDescriptorWithPixelFormat:width:height:mipmapped:" ,MTLPixelFormatDepth32Float,1,1,false);
        send<void>(dummyDepth,"setUsage:",MTLTextureUsageShaderRead); send<void>(dummyDepth,"setStorageMode:",MTLStorageModePrivate);
        v.depth3D=autoreleased(send<id>(v.device,"newTextureWithDescriptor:" ,dummyDepth));
        // Cache all ASCII and box/block glyphs once, instead of shaping text every frame.
        std::vector<uint8_t> bitmap(256*432);
        CGColorSpaceRef gray=CGColorSpaceCreateDeviceGray();
        CGContextRef context=CGBitmapContextCreate(bitmap.data(),256,432,8,256,gray,kCGImageAlphaNone);
        CGColorSpaceRelease(gray);
        CTFontRef font=CTFontCreateWithName(CFSTR("Menlo"),19,nullptr);
        CGContextSetGrayFillColor(context,1,1);
        CGContextSetShouldAntialias(context,true);
        for(int i=0;i<288;++i) {
            UniChar ch=i<128?UniChar(i):UniChar(0x2500+i-128);
            if(ch<32||ch==127) continue;
            CGGlyph glyph=0; CTFontGetGlyphsForCharacters(font,&ch,&glyph,1);
            CGSize advance{}; CTFontGetAdvancesForGlyphs(font,kCTFontOrientationHorizontal,&glyph,&advance,1);
            // CoreGraphics draws from the bottom; Metal texture rows start at the top.
            // Flip the atlas tile placement, not just each glyph within its tile.
            CGPoint point=CGPointMake((i%16)*16+(16-advance.width)/2,432-(i/16+1)*24+5);
            CTFontDrawGlyphs(font,&glyph,&point,1,context);
        }
        CFRelease(font); CGContextRelease(context);
        auto texture=send<id>(klass("MTLTextureDescriptor"),"texture2DDescriptorWithPixelFormat:width:height:mipmapped:" ,MTLPixelFormatR8Unorm,256,432,false);
        v.atlas=autoreleased(send<id>(v.device,"newTextureWithDescriptor:" ,texture));
        send<void>(v.atlas,"replaceRegion:mipmapLevel:withBytes:bytesPerRow:" ,MTLRegionMake2D(0,0,256,432),0,bitmap.data(),256);
        NSRect available=send<NSRect>(send<id>(klass("NSScreen"),"mainScreen"),"visibleFrame");
        NSRect rect=NSMakeRect(0,0,std::min(1280.0,available.size.width-40),std::min(800.0,available.size.height-70));
        v.window=autoreleased(send<id>(send<id>(klass("ERWindow"),"alloc"),"initWithContentRect:styleMask:backing:defer:" ,rect,NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskResizable|NSWindowStyleMaskMiniaturizable,NSBackingStoreBuffered,false));
        send<void>(v.window,"setReleasedWhenClosed:",false);
        send<void>(v.window,"setTitle:",str("Eternal"));
        send<void>(v.window,"setContentMinSize:",NSMakeSize(640,400));
        v.layer=send<id>(klass("CAMetalLayer"),"layer"); send<void>(v.layer,"setDevice:",v.device);
        send<void>(v.layer,"setPixelFormat:",MTLPixelFormatBGRA8Unorm); send<void>(v.layer,"setFramebufferOnly:",std::getenv("ER_CAPTURE")?false:true);
        send<void>(v.layer,"setMaximumDrawableCount:",2); send<void>(v.layer,"setDisplaySyncEnabled:",true);
        send<void>(v.window,"setContentView:",autoreleased(send<id>(send<id>(klass("ERGameView"),"alloc"),"initWithFrame:" ,NSMakeRect(0,0,rect.size.width,rect.size.height))));
        send<void>(send<id>(v.window,"contentView"),"setWantsLayer:",true); send<void>(send<id>(v.window,"contentView"),"setLayer:",v.layer);
        send<id>(NSApp,"finishLaunching");
        send<id>(NSApp,"activateIgnoringOtherApps:" ,true);
        send<id>(send<id>(klass("NSRunningApplication"),"currentApplication"),"activateWithOptions:" ,0);
        send<id>(v.window,"center"); send<id>(v.window,"makeKeyAndOrderFront:" ,nullptr);
        send<id>(v.window,"makeMainWindow"); send<id>(v.window,"makeFirstResponder:" ,send<id>(v.window,"contentView"));
    }
}
Window::~Window() { { Pool pool; if(impl->mouseCaptured) { CGAssociateMouseAndMouseCursorPosition(true); send<id>(klass("NSCursor"),"unhide"); } send<void>(impl->pending,"waitUntilCompleted"); send<id>(impl->window,"close"); } }
std::pair<int,int> Window::size(int detail) const {
    NSSize s=send<NSRect>(send<id>(impl->window,"contentView"),"bounds").size;
    return {clamp(int(s.width/(detail==0?6:detail==2?3:4)),40,500),clamp(int(s.height/(detail==0?12:detail==2?6:8)),16,180)};
}
std::pair<int,int> Window::interfaceSize() const {
    NSSize s=send<NSRect>(send<id>(impl->window,"contentView"),"bounds").size;
    return {clamp(int(s.width/8),40,250),clamp(int(s.height/16),16,100)};
}
Input Window::poll(bool menu,bool textEntry,bool firstPerson) {
    { Pool pool;
        Input input; input.heldMovement=true; auto& v=*impl;
        bool capture=firstPerson&&!menu&&send<bool>(v.window,"isKeyWindow");
        if(capture!=v.mouseCaptured) {
            v.mouseCaptured=capture; CGAssociateMouseAndMouseCursorPosition(!capture);
            if(capture) send<id>(klass("NSCursor"),"hide"); else send<id>(klass("NSCursor"),"unhide");
            send<void>(v.window,"setAcceptsMouseMovedEvents:",capture);
        }
        id  e; NSPoint clickPoint{}; bool clicked=false;
        while((e=send<id>(NSApp,"nextEventMatchingMask:untilDate:inMode:dequeue:" ,NSEventMaskAny,send<id>(klass("NSDate"),"distantPast"),NSDefaultRunLoopMode,true))) {
            bool consumed=false;
            if(send(e,"window")==v.window) {
                if(send<NSUInteger>(e,"type")==NSEventTypeKeyDown||send<NSUInteger>(e,"type")==NSEventTypeKeyUp) {
                    id  chars=send<id>(e,"charactersIgnoringModifiers");
                    unichar key=send<NSUInteger>(chars,"length")?send<unichar>(chars,"characterAtIndex:" ,0):0;
                    if(!textEntry&&key>='A'&&key<='Z') key+=32;
                    // Always process key-up, even if a modifier changed while held.
                    char movement=send<unsigned short>(e,"keyCode")==13?'w':send<unsigned short>(e,"keyCode")==0?'a':send<unsigned short>(e,"keyCode")==1?'s':send<unsigned short>(e,"keyCode")==2?'d':0;
                    if(movement&&send<NSUInteger>(e,"type")==NSEventTypeKeyUp) v.held[size_t(movement)]=false;
                    if(!(send<NSUInteger>(e,"modifierFlags")&NSEventModifierFlagCommand)) {
                        if(movement) v.held[size_t(movement)]=send<NSUInteger>(e,"type")==NSEventTypeKeyDown;
                        if(send<NSUInteger>(e,"type")==NSEventTypeKeyDown) {
                            if(!menu&&key==NSF8FunctionKey) { v.raw=!v.raw; consumed=true; continue; }
                            if(!menu&&key==NSF9FunctionKey) { v.debug=!v.debug; consumed=true; continue; }
                            if(key==NSUpArrowFunctionKey) input.keys+='^';
                            else if(key==NSDownArrowFunctionKey) input.keys+='v';
                            else if(key==NSLeftArrowFunctionKey) input.keys+='<';
                            else if(key==NSRightArrowFunctionKey) input.keys+='>';
                            else if(key<128&&!(key==32&&!menu&&send<bool>(e,"isARepeat"))) input.keys+=char(key);
                        }
                        consumed=true;
                    }
                }
                if(send<NSUInteger>(e,"type")==NSEventTypeLeftMouseDown) {
                    send<id>(NSApp,"activateIgnoringOtherApps:" ,true);
                    send<id>(v.window,"makeKeyAndOrderFront:" ,nullptr);
                    send<id>(v.window,"makeFirstResponder:" ,send<id>(v.window,"contentView"));
                    clickPoint=send<NSPoint>(send<id>(v.window,"contentView"),"convertPoint:fromView:" ,send<NSPoint>(e,"locationInWindow"),nullptr);
                    clicked=true; input.click=true; consumed=true;
                }
                if(!menu&&(send<NSUInteger>(e,"type")==NSEventTypeRightMouseDragged||(v.mouseCaptured&&(send<NSUInteger>(e,"type")==NSEventTypeMouseMoved||send<NSUInteger>(e,"type")==NSEventTypeLeftMouseDragged)))) { input.orbitX+=send<double>(e,"deltaX"); input.orbitY+=send<double>(e,"deltaY"); consumed=true; }
                if(send<NSUInteger>(e,"type")==NSEventTypeLeftMouseUp) consumed=true;
                if(send<NSUInteger>(e,"type")==NSEventTypeScrollWheel) input.scroll+=send<double>(e,"scrollingDeltaY")>0?1:send<double>(e,"scrollingDeltaY")<0?-1:0;
            }
            if(!consumed) send<id>(NSApp,"sendEvent:" ,e);
        }
        if(!send<bool>(v.window,"isKeyWindow")) v.held.fill(false);
        input.sprint=send<bool>(v.window,"isKeyWindow")&&!menu&&(send<NSUInteger>(klass("NSEvent"),"modifierFlags")&NSEventModifierFlagShift);
        // Hardware key-up events remove the terminal keyboard-repeat delay.
        for(char k:std::string("wasd")) if(v.held[size_t(k)]) input.movementKeys+=k;
        if(send<bool>(v.window,"isKeyWindow")||clicked) {
            NSPoint mouse=clicked?clickPoint:send<NSPoint>(send<id>(v.window,"contentView"),"convertPoint:fromView:" ,send<NSPoint>(v.window,"mouseLocationOutsideOfEventStream"),nullptr);
            auto [cols,rows]=std::pair<int,int>{menu?v.uiCols:v.cols,menu?v.uiRows:v.rows}; NSSize bounds=send<NSRect>(send<id>(v.window,"contentView"),"bounds").size;
            if(mouse.x>=0&&mouse.y>=0&&mouse.x<bounds.width&&mouse.y<bounds.height) {
                input.mouseX=int(mouse.x/bounds.width*cols);
                input.mouseY=int((bounds.height-mouse.y)/bounds.height*rows);
            }
        }
        if(v.mouseCaptured) { input.mouseX=v.cols/2; input.mouseY=v.rows/2; }
        if(!send<bool>(v.window,"isVisible")) input.keys+='q';
        return input;
    }
}
bool Window::ready() const { return !impl->pending||send<NSUInteger>(impl->pending,"status")>=MTLCommandBufferStatusCompleted; }
void Window::raw3D(bool enabled) { impl->raw=enabled; }
bool Window::raw3D() const { return impl->raw; }
bool Window::mouseCaptured() const { return impl->mouseCaptured; }
bool Window::debug3D() const { return impl->debug; }
double Window::gpuMilliseconds() const { auto p=impl->pending; return p&&send<NSUInteger>(p,"status")==MTLCommandBufferStatusCompleted?(send<double>(p,"GPUEndTime")-send<double>(p,"GPUStartTime"))*1000:0; }
double Window::terrainMilliseconds() const { auto p=impl->pendingTerrain; return p&&send<NSUInteger>(p,"status")==MTLCommandBufferStatusCompleted?(send<double>(p,"GPUEndTime")-send<double>(p,"GPUStartTime"))*1000:0; }
bool Window::present(const Framebuffer& frame,const Framebuffer& interface,const Terrain3D* terrain,const Camera* camera,float backdrop) {
    { Pool pool;
        auto& v=*impl;
        v.cols=frame.width; v.rows=frame.height; v.uiCols=interface.width; v.uiRows=interface.height;
        // At most one GPU frame in flight; skip stale presentation opportunities.
        if(v.pending&&send<NSUInteger>(v.pending,"status")<MTLCommandBufferStatusCompleted) return false;
        if(send<NSUInteger>(v.pending,"status")==MTLCommandBufferStatusError) throw std::runtime_error("Metal frame failed");
        NSSize bounds=send<NSRect>(send<id>(v.window,"contentView"),"bounds").size;
        CGFloat scale=send<double>(v.window,"backingScaleFactor");
        CGSize drawableSize=CGSizeMake(bounds.width*scale,bounds.height*scale);
        if(!CGSizeEqualToSize(send<NSSize>(v.layer,"drawableSize"),drawableSize)) send<void>(v.layer,"setDrawableSize:",drawableSize);
        id drawable=send<id>(v.layer,"nextDrawable");
        if(!drawable) return false;
        struct GPUCell { uint32_t glyph,fg,bg; float depth; };
        auto upload=[&](const Framebuffer& source,Object& buffer) {
            size_t bytes=source.cells.size()*sizeof(GPUCell);
            if(!buffer||send<NSUInteger>(buffer,"length")<bytes) buffer=autoreleased(send<id>(v.device,"newBufferWithLength:options:" ,bytes,MTLResourceStorageModeShared));
            auto* cells=static_cast<GPUCell*>(send<void*>(buffer,"contents"));
            auto packed=[](Color c){return uint32_t(c.r)|(uint32_t(c.g)<<8)|(uint32_t(c.b)<<16);};
            for(size_t i=0;i<source.cells.size();++i) {
                const auto& c=source.cells[i]; uint32_t g=c.glyph;
                g=g<128?g:(g>=0x2500&&g<0x25a0?128+g-0x2500:uint32_t('?'));
                cells[i]={g,packed(c.fg),packed(c.bg),c.depth};
            }
        };
        upload(frame,v.cells); upload(interface,v.interface);
        auto command=send<id>(v.queue,"commandBuffer");
        bool use3D=terrain&&camera;
        if(use3D) {
            if(!v.color3D||send<NSUInteger>(v.color3D,"width")!=NSUInteger(frame.width)||send<NSUInteger>(v.color3D,"height")!=NSUInteger(frame.height)) {
                auto texture=send<id>(klass("MTLTextureDescriptor"),"texture2DDescriptorWithPixelFormat:width:height:mipmapped:" ,MTLPixelFormatRGBA16Float,frame.width,frame.height,false);
                send<void>(texture,"setUsage:",MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead); send<void>(texture,"setStorageMode:",MTLStorageModePrivate);
                v.color3D=autoreleased(send<id>(v.device,"newTextureWithDescriptor:" ,texture)); v.normal3D=autoreleased(send<id>(v.device,"newTextureWithDescriptor:" ,texture));
                send<void>(texture,"setPixelFormat:",MTLPixelFormatDepth32Float); v.depth3D=autoreleased(send<id>(v.device,"newTextureWithDescriptor:" ,texture));
            }
            if(v.meshRevision!=terrain->revision) {
                v.meshVertices=0; for(const auto& chunk:terrain->chunks) v.meshVertices+=chunk.second.vertices.size();
                size_t size=v.meshVertices*sizeof(TerrainVertex);
                if(!v.meshBuffer||send<NSUInteger>(v.meshBuffer,"length")<size) v.meshBuffer=autoreleased(send<id>(v.device,"newBufferWithLength:options:" ,std::max(size,size_t(1)),MTLResourceStorageModeShared));
                auto* target=static_cast<TerrainVertex*>(send<void*>(v.meshBuffer,"contents"));
                for(const auto& chunk:terrain->chunks) { std::copy(chunk.second.vertices.begin(),chunk.second.vertices.end(),target); target+=chunk.second.vertices.size(); }
                v.meshRevision=terrain->revision;
            }
            auto terrainPass=send<id>(klass("MTLRenderPassDescriptor"),"renderPassDescriptor");
            send<void>(at(send<id>(terrainPass,"colorAttachments"),0),"setTexture:",v.color3D); send<void>(at(send<id>(terrainPass,"colorAttachments"),1),"setTexture:",v.normal3D);
            for(int i=0;i<2;++i) { send<void>(at(send<id>(terrainPass,"colorAttachments"),i),"setLoadAction:",MTLLoadActionClear); send<void>(at(send<id>(terrainPass,"colorAttachments"),i),"setStoreAction:",MTLStoreActionStore); }
            send<void>(at(send<id>(terrainPass,"colorAttachments"),0),"setClearColor:",MTLClearColorMake(.043,.043,.102,1));
            send<void>(send<id>(terrainPass,"depthAttachment"),"setTexture:",v.depth3D); send<void>(send<id>(terrainPass,"depthAttachment"),"setLoadAction:",MTLLoadActionClear);
            send<void>(send<id>(terrainPass,"depthAttachment"),"setStoreAction:",MTLStoreActionStore); send<void>(send<id>(terrainPass,"depthAttachment"),"setClearDepth:",1.0);
            auto terrainCommand=send<id>(v.queue,"commandBuffer");
            auto encoder=send<id>(terrainCommand,"renderCommandEncoderWithDescriptor:" ,terrainPass);
            send<void>(encoder,"setRenderPipelineState:" ,v.terrainPipeline); send<void>(encoder,"setDepthStencilState:" ,v.depthState);
            send<void>(encoder,"setVertexBuffer:offset:atIndex:" ,v.meshBuffer,0,0); auto view=camera->view();
            send<void>(encoder,"setVertexBytes:length:atIndex:" ,&view,sizeof(view),1);
            send<void>(encoder,"drawPrimitives:vertexStart:vertexCount:" ,MTLPrimitiveTypeTriangle,0,v.meshVertices); send<void>(encoder,"endEncoding");
            send<void>(terrainCommand,"commit"); v.pendingTerrain=terrainCommand;
            size_t cellBytes=size_t(frame.width)*frame.height*sizeof(GPUCell);
            if(!v.terrainCells||send<NSUInteger>(v.terrainCells,"length")<cellBytes)
                v.terrainCells=autoreleased(send<id>(v.device,"newBufferWithLength:options:" ,cellBytes,MTLResourceStorageModePrivate));
            auto convert=send<id>(command,"computeCommandEncoder");
            send<void>(convert,"setComputePipelineState:" ,v.asciiPipeline);
            send<void>(convert,"setTexture:atIndex:" ,v.color3D,0); send<void>(convert,"setTexture:atIndex:" ,v.normal3D,1); send<void>(convert,"setTexture:atIndex:" ,v.depth3D,2);
            send<void>(convert,"setBuffer:offset:atIndex:" ,v.terrainCells,0,0); send<void>(convert,"setBytes:length:atIndex:" ,&view,sizeof(view),1);
            send<void>(convert,"dispatchThreads:threadsPerThreadgroup:" ,MTLSizeMake(frame.width,frame.height,1),MTLSizeMake(8,8,1));
            send<void>(convert,"endEncoding");
        }
        auto pass=send<id>(klass("MTLRenderPassDescriptor"),"renderPassDescriptor");
        send<void>(at(send<id>(pass,"colorAttachments"),0),"setTexture:",send<id>(drawable,"texture"));
        send<void>(at(send<id>(pass,"colorAttachments"),0),"setLoadAction:",MTLLoadActionDontCare);
        send<void>(at(send<id>(pass,"colorAttachments"),0),"setStoreAction:",MTLStoreActionStore);
        auto encoder=send<id>(command,"renderCommandEncoderWithDescriptor:" ,pass);
        send<void>(encoder,"setRenderPipelineState:" ,v.pipeline);
        send<void>(encoder,"setFragmentBuffer:offset:atIndex:" ,v.cells,0,0);
        send<void>(encoder,"setFragmentBuffer:offset:atIndex:" ,use3D?v.terrainCells:v.cells,0,6);
        uint32_t grid[]={uint32_t(frame.width),uint32_t(frame.height)};
        send<void>(encoder,"setFragmentBytes:length:atIndex:" ,grid,sizeof(grid),1);
        send<void>(encoder,"setFragmentTexture:atIndex:" ,v.atlas,0);
        send<void>(encoder,"setFragmentTexture:atIndex:" ,use3D?v.color3D:v.atlas,1);
        send<void>(encoder,"setFragmentTexture:atIndex:" ,v.depth3D,2);
        send<void>(encoder,"setFragmentBytes:length:atIndex:" ,&backdrop,sizeof(backdrop),5);
        uint32_t mode=use3D?(v.raw?2:1):0; send<void>(encoder,"setFragmentBytes:length:atIndex:" ,&mode,sizeof(mode),4);
        send<void>(encoder,"setFragmentBuffer:offset:atIndex:" ,v.interface,0,2);
        uint32_t uiGrid[]={uint32_t(interface.width),uint32_t(interface.height)};
        send<void>(encoder,"setFragmentBytes:length:atIndex:" ,uiGrid,sizeof(uiGrid),3);
        send<void>(encoder,"drawPrimitives:vertexStart:vertexCount:" ,MTLPrimitiveTypeTriangle,0,3);
        send<void>(encoder,"endEncoding");
        const char* capture=std::getenv("ER_CAPTURE");
        Object pixels;
        NSUInteger width=send<NSUInteger>(send<id>(drawable,"texture"),"width"),height=send<NSUInteger>(send<id>(drawable,"texture"),"height"),stride=((width*4+255)/256)*256;
        if(capture) {
            pixels=autoreleased(send<id>(v.device,"newBufferWithLength:options:" ,stride*height,MTLResourceStorageModeShared));
            auto copy=send<id>(command,"blitCommandEncoder");
            send<void>(copy,"copyFromTexture:sourceSlice:sourceLevel:sourceOrigin:sourceSize:toBuffer:destinationOffset:destinationBytesPerRow:destinationBytesPerImage:" ,send<id>(drawable,"texture"),0,0,MTLOriginMake(0,0,0),MTLSizeMake(width,height,1),pixels,0,stride,stride*height);
            send<void>(copy,"endEncoding");
        }
        send<void>(command,"presentDrawable:" ,drawable); send<void>(command,"commit");
        if(capture) {
            send<void>(command,"waitUntilCompleted");
            CGColorSpaceRef color=CGColorSpaceCreateDeviceRGB();
            CGContextRef context=CGBitmapContextCreate(send<void*>(pixels,"contents"),width,height,8,stride,color,kCGBitmapByteOrder32Little|kCGImageAlphaNoneSkipFirst);
            CGImageRef image=CGBitmapContextCreateImage(context);
            id  bitmap=autoreleased(send<id>(send<id>(klass("NSBitmapImageRep"),"alloc"),"initWithCGImage:" ,image));
            send<id>(send<id>(bitmap,"representationUsingType:properties:" ,NSBitmapImageFileTypePNG,send(klass("NSDictionary"),"dictionary")),"writeToFile:atomically:" ,send<id>(klass("NSString"),"stringWithUTF8String:" ,capture),true);
            CGImageRelease(image); CGContextRelease(context); CGColorSpaceRelease(color);
            unsetenv("ER_CAPTURE");
        }
        v.pending=command;
        return true;
    }
}
}
