#include "platform/window.hpp"
#import <AppKit/AppKit.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <CoreText/CoreText.h>
#include <array>
#include <stdexcept>

@interface ERWindow : NSWindow
@end
@implementation ERWindow
- (BOOL)canBecomeKeyWindow { return YES; }
@end

namespace er {
struct Window::Impl {
    NSWindow* window;
    CAMetalLayer* layer;
    id<MTLDevice> device;
    id<MTLCommandQueue> queue;
    id<MTLRenderPipelineState> pipeline;
    id<MTLTexture> atlas;
    id<MTLBuffer> cells;
    id<MTLCommandBuffer> pending;
    std::array<bool,128> held{};
    int cols=320,rows=100;
};
static const char* shader=R"(
#include <metal_stdlib>
using namespace metal;
struct Cell { uint glyph; uint fg; uint bg; uint pad; };
struct Vertex { float4 position [[position]]; float2 uv; };
vertex Vertex vertices(uint id [[vertex_id]]) {
    float2 p=float2((id==1)?3.0:-1.0,(id==2)?3.0:-1.0);
    return {float4(p,0,1),float2((p.x+1)*0.5,(1-p.y)*0.5)};
}
float3 rgb(uint v) { return float3(v&255,(v>>8)&255,(v>>16)&255)/255.0; }
fragment float4 draw(Vertex in [[stage_in]], constant Cell* cells [[buffer(0)]],
                     constant uint2& grid [[buffer(1)]], texture2d<float> atlas [[texture(0)]]) {
    float2 p=clamp(in.uv,float2(0),float2(0.999999))*float2(grid);
    uint2 cell=uint2(p);
    Cell c=cells[cell.y*grid.x+cell.x];
    float2 tile=float2(c.glyph%16,c.glyph/16);
    float2 local=fract(p); local.y=1-local.y;
    constexpr sampler sample(filter::linear,address::clamp_to_edge);
    float a=atlas.sample(sample,(tile*float2(16,24)+float2(0.5)+local*float2(15,23))/float2(256,432)).r;
    return float4(mix(rgb(c.bg),rgb(c.fg),a),1);
}
)";
Window::Window():impl(std::make_unique<Impl>()) {
    @autoreleasepool {
        auto& v=*impl;
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        v.device=MTLCreateSystemDefaultDevice();
        if(!v.device) throw std::runtime_error("Metal GPU unavailable; run without --window");
        NSError* error=nil;
        id<MTLLibrary> library=[v.device newLibraryWithSource:[NSString stringWithUTF8String:shader] options:nil error:&error];
        if(!library) throw std::runtime_error([[error localizedDescription] UTF8String]);
        MTLRenderPipelineDescriptor* descriptor=[MTLRenderPipelineDescriptor new];
        descriptor.vertexFunction=[library newFunctionWithName:@"vertices"];
        descriptor.fragmentFunction=[library newFunctionWithName:@"draw"];
        descriptor.colorAttachments[0].pixelFormat=MTLPixelFormatBGRA8Unorm;
        v.pipeline=[v.device newRenderPipelineStateWithDescriptor:descriptor error:&error];
        if(!v.pipeline) throw std::runtime_error([[error localizedDescription] UTF8String]);
        v.queue=[v.device newCommandQueue];
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
            CGPoint point=CGPointMake((i%16)*16+(16-advance.width)/2,(i/16)*24+5);
            CTFontDrawGlyphs(font,&glyph,&point,1,context);
        }
        CFRelease(font); CGContextRelease(context);
        auto texture=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatR8Unorm width:256 height:432 mipmapped:NO];
        v.atlas=[v.device newTextureWithDescriptor:texture];
        [v.atlas replaceRegion:MTLRegionMake2D(0,0,256,432) mipmapLevel:0 withBytes:bitmap.data() bytesPerRow:256];
        NSRect available=[[NSScreen mainScreen] visibleFrame];
        NSRect rect=NSMakeRect(0,0,std::min(1280.0,available.size.width-40),std::min(800.0,available.size.height-70));
        v.window=[[ERWindow alloc] initWithContentRect:rect styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskResizable|NSWindowStyleMaskMiniaturizable backing:NSBackingStoreBuffered defer:NO];
        v.window.releasedWhenClosed=NO;
        v.window.title=@"EternalRPG — Metal ASCII";
        v.window.contentMinSize=NSMakeSize(640,400);
        v.layer=[CAMetalLayer layer]; v.layer.device=v.device;
        v.layer.pixelFormat=MTLPixelFormatBGRA8Unorm; v.layer.framebufferOnly=YES;
        v.layer.maximumDrawableCount=2; v.layer.displaySyncEnabled=YES;
        v.window.contentView.wantsLayer=YES; v.window.contentView.layer=v.layer;
        [v.window center]; [v.window makeKeyAndOrderFront:nil];
        [NSApp finishLaunching]; [NSApp activateIgnoringOtherApps:YES];
    }
}
Window::~Window() { @autoreleasepool { [impl->pending waitUntilCompleted]; [impl->window close]; } }
std::pair<int,int> Window::size(int detail) const {
    NSSize s=impl->window.contentView.bounds.size;
    return {clamp(int(s.width/(detail==0?6:detail==2?3:4)),40,500),clamp(int(s.height/(detail==0?12:detail==2?6:8)),16,180)};
}
Input Window::poll() {
    @autoreleasepool {
        Input input; input.heldMovement=true; auto& v=*impl;
        NSEvent* e;
        while((e=[NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES])) {
            bool consumed=false;
            if(e.window==v.window) {
                if(e.type==NSEventTypeKeyDown||e.type==NSEventTypeKeyUp) {
                    NSString* chars=[e charactersIgnoringModifiers];
                    unichar key=chars.length?[chars characterAtIndex:0]:0;
                    if(key>='A'&&key<='Z') key+=32;
                    // Always process key-up, even if a modifier changed while held.
                    char movement=e.keyCode==13?'w':e.keyCode==0?'a':e.keyCode==1?'s':e.keyCode==2?'d':0;
                    if(movement&&e.type==NSEventTypeKeyUp) v.held[size_t(movement)]=false;
                    if(!(e.modifierFlags&NSEventModifierFlagCommand)) {
                        if(movement) v.held[size_t(movement)]=e.type==NSEventTypeKeyDown;
                        if(e.type==NSEventTypeKeyDown) {
                            if(key==NSUpArrowFunctionKey) input.keys+='^';
                            else if(key==NSDownArrowFunctionKey) input.keys+='v';
                            else if(key==NSLeftArrowFunctionKey) input.keys+='<';
                            else if(key==NSRightArrowFunctionKey) input.keys+='>';
                            else if(key<128) input.keys+=char(key);
                        }
                        consumed=true;
                    }
                }
                if(e.type==NSEventTypeLeftMouseDown) input.click=true;
                if(e.type==NSEventTypeScrollWheel) input.scroll+=e.scrollingDeltaY>0?1:e.scrollingDeltaY<0?-1:0;
            }
            if(!consumed) [NSApp sendEvent:e];
        }
        if(!v.window.keyWindow) v.held.fill(false);
        // Hardware key-up events remove the terminal keyboard-repeat delay.
        for(char k:std::string("wasd")) if(v.held[size_t(k)]) input.movementKeys+=k;
        if(v.window.keyWindow) {
            NSPoint mouse=[v.window mouseLocationOutsideOfEventStream];
            auto [cols,rows]=std::pair<int,int>{v.cols,v.rows}; NSSize bounds=v.window.contentView.bounds.size;
            if(mouse.x>=0&&mouse.y>=0&&mouse.x<bounds.width&&mouse.y<bounds.height) {
                input.mouseX=int(mouse.x/bounds.width*cols);
                input.mouseY=int((bounds.height-mouse.y)/bounds.height*rows);
            }
        }
        if(!v.window.visible) input.keys+='q';
        return input;
    }
}
bool Window::ready() const { return !impl->pending||impl->pending.status>=MTLCommandBufferStatusCompleted; }
bool Window::present(const Framebuffer& frame) {
    @autoreleasepool {
        auto& v=*impl;
        v.cols=frame.width; v.rows=frame.height;
        // At most one GPU frame in flight; skip stale presentation opportunities.
        if(v.pending&&v.pending.status<MTLCommandBufferStatusCompleted) return false;
        if(v.pending.status==MTLCommandBufferStatusError) throw std::runtime_error("Metal frame failed");
        NSSize bounds=v.window.contentView.bounds.size;
        CGFloat scale=v.window.backingScaleFactor;
        CGSize drawableSize=CGSizeMake(bounds.width*scale,bounds.height*scale);
        if(!CGSizeEqualToSize(v.layer.drawableSize,drawableSize)) v.layer.drawableSize=drawableSize;
        id<CAMetalDrawable> drawable=[v.layer nextDrawable];
        if(!drawable) return false;
        struct GPUCell { uint32_t glyph,fg,bg,pad; };
        size_t bytes=frame.cells.size()*sizeof(GPUCell);
        if(!v.cells||v.cells.length<bytes) v.cells=[v.device newBufferWithLength:bytes options:MTLResourceStorageModeShared];
        auto* cells=static_cast<GPUCell*>(v.cells.contents);
        auto packed=[](Color c){return uint32_t(c.r)|(uint32_t(c.g)<<8)|(uint32_t(c.b)<<16);};
        for(size_t i=0;i<frame.cells.size();++i) {
            const auto& c=frame.cells[i]; uint32_t g=c.glyph;
            g=g<128?g:(g>=0x2500&&g<0x25a0?128+g-0x2500:uint32_t('?'));
            cells[i]={g,packed(c.fg),packed(c.bg),0};
        }
        auto pass=[MTLRenderPassDescriptor renderPassDescriptor];
        pass.colorAttachments[0].texture=drawable.texture;
        pass.colorAttachments[0].loadAction=MTLLoadActionDontCare;
        pass.colorAttachments[0].storeAction=MTLStoreActionStore;
        auto command=[v.queue commandBuffer]; auto encoder=[command renderCommandEncoderWithDescriptor:pass];
        [encoder setRenderPipelineState:v.pipeline];
        [encoder setFragmentBuffer:v.cells offset:0 atIndex:0];
        uint32_t grid[]={uint32_t(frame.width),uint32_t(frame.height)};
        [encoder setFragmentBytes:grid length:sizeof(grid) atIndex:1];
        [encoder setFragmentTexture:v.atlas atIndex:0];
        [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
        [encoder endEncoding]; [command presentDrawable:drawable]; [command commit];
        v.pending=command;
        return true;
    }
}
}
