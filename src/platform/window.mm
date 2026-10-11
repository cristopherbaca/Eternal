#include "platform/window.hpp"
#import <AppKit/AppKit.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <CoreText/CoreText.h>
#include <array>
#include <cstdlib>
#include <stdexcept>

// A real input view is needed for click-to-focus and keyboard responder routing.
@interface ERGameView : NSView
@end
@implementation ERGameView
- (BOOL)acceptsFirstResponder { return YES; }
- (BOOL)acceptsFirstMouse:(NSEvent*)event { (void)event; return YES; }
- (void)mouseDown:(NSEvent*)event { (void)event; [self.window makeFirstResponder:self]; }
@end

@interface ERWindow : NSWindow
@end
@implementation ERWindow
- (BOOL)canBecomeKeyWindow { return YES; }
- (BOOL)canBecomeMainWindow { return YES; }
@end

namespace er {
struct Window::Impl {
    NSWindow* window;
    CAMetalLayer* layer;
    id<MTLDevice> device;
    id<MTLCommandQueue> queue;
    id<MTLRenderPipelineState> pipeline;
    id<MTLTexture> atlas;
    id<MTLBuffer> cells,interface;
    id<MTLCommandBuffer> pending;
    std::array<bool,128> held{};
    int cols=320,rows=100,uiCols=160,uiRows=50;
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
                     constant uint2& grid [[buffer(1)]], texture2d<float> atlas [[texture(0)]],
                     constant Cell* interface [[buffer(2)]], constant uint2& uiGrid [[buffer(3)]]) {
    float2 p=clamp(in.uv,float2(0),float2(0.999999))*float2(grid);
    uint2 cell=uint2(p);
    Cell c=cells[cell.y*grid.x+cell.x];
    float2 ui=clamp(in.uv,float2(0),float2(0.999999))*float2(uiGrid);
    Cell overlay=interface[uint(ui.y)*uiGrid.x+uint(ui.x)];
    if(overlay.glyph!=0) { c=overlay; p=ui; }
    float2 tile=float2(c.glyph%16,c.glyph/16);
    float2 local=fract(p);
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
            // CoreGraphics draws from the bottom; Metal texture rows start at the top.
            // Flip the atlas tile placement, not just each glyph within its tile.
            CGPoint point=CGPointMake((i%16)*16+(16-advance.width)/2,432-(i/16+1)*24+5);
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
        v.layer.pixelFormat=MTLPixelFormatBGRA8Unorm; v.layer.framebufferOnly=std::getenv("ER_CAPTURE")?NO:YES;
        v.layer.maximumDrawableCount=2; v.layer.displaySyncEnabled=YES;
        v.window.contentView=[[ERGameView alloc] initWithFrame:NSMakeRect(0,0,rect.size.width,rect.size.height)];
        v.window.contentView.wantsLayer=YES; v.window.contentView.layer=v.layer;
        [NSApp finishLaunching];
        [NSApp activateIgnoringOtherApps:YES];
        [[NSRunningApplication currentApplication] activateWithOptions:0];
        [v.window center]; [v.window makeKeyAndOrderFront:nil];
        [v.window makeMainWindow]; [v.window makeFirstResponder:v.window.contentView];
    }
}
Window::~Window() { @autoreleasepool { [impl->pending waitUntilCompleted]; [impl->window close]; } }
std::pair<int,int> Window::size(int detail) const {
    NSSize s=impl->window.contentView.bounds.size;
    return {clamp(int(s.width/(detail==0?6:detail==2?3:4)),40,500),clamp(int(s.height/(detail==0?12:detail==2?6:8)),16,180)};
}
std::pair<int,int> Window::interfaceSize() const {
    NSSize s=impl->window.contentView.bounds.size;
    return {clamp(int(s.width/8),40,250),clamp(int(s.height/16),16,100)};
}
Input Window::poll(bool menu,bool textEntry) {
    @autoreleasepool {
        Input input; input.heldMovement=true; auto& v=*impl;
        NSEvent* e; NSPoint clickPoint{}; bool clicked=false;
        while((e=[NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES])) {
            bool consumed=false;
            if(e.window==v.window) {
                if(e.type==NSEventTypeKeyDown||e.type==NSEventTypeKeyUp) {
                    NSString* chars=[e charactersIgnoringModifiers];
                    unichar key=chars.length?[chars characterAtIndex:0]:0;
                    if(!textEntry&&key>='A'&&key<='Z') key+=32;
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
                if(e.type==NSEventTypeLeftMouseDown) {
                    [NSApp activateIgnoringOtherApps:YES];
                    [v.window makeKeyAndOrderFront:nil];
                    [v.window makeFirstResponder:v.window.contentView];
                    clickPoint=[v.window.contentView convertPoint:e.locationInWindow fromView:nil];
                    clicked=true; input.click=true; consumed=true;
                }
                if(e.type==NSEventTypeLeftMouseUp) consumed=true;
                if(e.type==NSEventTypeScrollWheel) input.scroll+=e.scrollingDeltaY>0?1:e.scrollingDeltaY<0?-1:0;
            }
            if(!consumed) [NSApp sendEvent:e];
        }
        if(!v.window.keyWindow) v.held.fill(false);
        // Hardware key-up events remove the terminal keyboard-repeat delay.
        for(char k:std::string("wasd")) if(v.held[size_t(k)]) input.movementKeys+=k;
        if(v.window.keyWindow||clicked) {
            NSPoint mouse=clicked?clickPoint:[v.window.contentView convertPoint:[v.window mouseLocationOutsideOfEventStream] fromView:nil];
            auto [cols,rows]=std::pair<int,int>{menu?v.uiCols:v.cols,menu?v.uiRows:v.rows}; NSSize bounds=v.window.contentView.bounds.size;
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
bool Window::present(const Framebuffer& frame,const Framebuffer& interface) {
    @autoreleasepool {
        auto& v=*impl;
        v.cols=frame.width; v.rows=frame.height; v.uiCols=interface.width; v.uiRows=interface.height;
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
        auto upload=[&](const Framebuffer& source,id<MTLBuffer> __strong& buffer) {
            size_t bytes=source.cells.size()*sizeof(GPUCell);
            if(!buffer||buffer.length<bytes) buffer=[v.device newBufferWithLength:bytes options:MTLResourceStorageModeShared];
            auto* cells=static_cast<GPUCell*>(buffer.contents);
            auto packed=[](Color c){return uint32_t(c.r)|(uint32_t(c.g)<<8)|(uint32_t(c.b)<<16);};
            for(size_t i=0;i<source.cells.size();++i) {
                const auto& c=source.cells[i]; uint32_t g=c.glyph;
                g=g<128?g:(g>=0x2500&&g<0x25a0?128+g-0x2500:uint32_t('?'));
                cells[i]={g,packed(c.fg),packed(c.bg),0};
            }
        };
        upload(frame,v.cells); upload(interface,v.interface);
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
        [encoder setFragmentBuffer:v.interface offset:0 atIndex:2];
        uint32_t uiGrid[]={uint32_t(interface.width),uint32_t(interface.height)};
        [encoder setFragmentBytes:uiGrid length:sizeof(uiGrid) atIndex:3];
        [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
        [encoder endEncoding];
        const char* capture=std::getenv("ER_CAPTURE");
        id<MTLBuffer> pixels=nil;
        NSUInteger width=drawable.texture.width,height=drawable.texture.height,stride=((width*4+255)/256)*256;
        if(capture) {
            pixels=[v.device newBufferWithLength:stride*height options:MTLResourceStorageModeShared];
            auto copy=[command blitCommandEncoder];
            [copy copyFromTexture:drawable.texture sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0,0,0)
                       sourceSize:MTLSizeMake(width,height,1) toBuffer:pixels destinationOffset:0
              destinationBytesPerRow:stride destinationBytesPerImage:stride*height];
            [copy endEncoding];
        }
        [command presentDrawable:drawable]; [command commit];
        if(capture) {
            [command waitUntilCompleted];
            CGColorSpaceRef color=CGColorSpaceCreateDeviceRGB();
            CGContextRef context=CGBitmapContextCreate(pixels.contents,width,height,8,stride,color,kCGBitmapByteOrder32Little|kCGImageAlphaNoneSkipFirst);
            CGImageRef image=CGBitmapContextCreateImage(context);
            NSBitmapImageRep* bitmap=[[NSBitmapImageRep alloc] initWithCGImage:image];
            [[bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}] writeToFile:[NSString stringWithUTF8String:capture] atomically:YES];
            CGImageRelease(image); CGContextRelease(context); CGColorSpaceRelease(color);
            unsetenv("ER_CAPTURE");
        }
        v.pending=command;
        return true;
    }
}
}
