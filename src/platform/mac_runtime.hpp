#pragma once
// C++17 bridge to Apple's system frameworks through their public C runtime ABI.
// No Objective-C/Objective-C++ compiler or language syntax is used by the host.
#include <objc/runtime.h>
#include <objc/message.h>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>
#include <utility>
#include <type_traits>
using NSUInteger=unsigned long;
using unichar=unsigned short;
using NSRect=CGRect; using NSSize=CGSize; using NSPoint=CGPoint;
inline NSRect NSMakeRect(double x,double y,double w,double h) { return CGRectMake(x,y,w,h); }
inline NSSize NSMakeSize(double w,double h) { return CGSizeMake(w,h); }
inline NSPoint NSMakePoint(double x,double y) { return CGPointMake(x,y); }
inline constexpr NSPoint NSZeroPoint{0,0};
extern "C" { extern id NSApp; extern id const NSDefaultRunLoopMode; id MTLCreateSystemDefaultDevice(); }
namespace er::mac {
class Object;
inline id raw(const Object&);
// Cocoa/Metal indices and enum parameters use pointer-width integers. Widen
// literals too: ARM64 stack arguments otherwise leave the upper word undefined.
template<class T> auto raw(T value) {
    if constexpr(std::is_integral_v<T>&&!std::is_same_v<T,bool>) return static_cast<NSUInteger>(value);
    else return value;
}
template<class R=id,class... A> R send(id target,const char* selector,A... args) {
    if(!target) { if constexpr(!std::is_void_v<R>) return R{}; else return; }
    SEL method=sel_registerName(selector);
#if defined(__x86_64__)
    if constexpr(std::is_same_v<R,CGRect>) {
        R result{};
        reinterpret_cast<void(*)(R*,id,SEL,decltype(raw(args))...)>(objc_msgSend_stret)(&result,target,method,raw(args)...);
        return result;
    } else
#endif
    return reinterpret_cast<R(*)(id,SEL,decltype(raw(args))...)>(objc_msgSend)(target,method,raw(args)...);
}
inline id klass(const char* name) { return reinterpret_cast<id>(objc_getClass(name)); }
inline id autoreleased(id value) { return send(value,"autorelease"); }
inline id str(const char* value) { return send(klass("NSString"),"stringWithUTF8String:",value); }
inline id at(id value,NSUInteger index) { return send(value,"objectAtIndexedSubscript:",index); }
class Object {
    id value=nullptr;
public:
    Object()=default;
    Object(id p):value(send(p,"retain")) {}
    Object(const Object& other):Object(other.value) {}
    Object(Object&& other) noexcept:value(std::exchange(other.value,nullptr)) {}
    ~Object() { send<void>(value,"release"); }
    Object& operator=(Object other) noexcept { std::swap(value,other.value); return *this; }
    operator id() const { return value; }
};
inline id raw(const Object& value) { return value; }
struct Pool {
    id value=send(klass("NSAutoreleasePool"),"new");
    ~Pool() { send<void>(value,"drain"); }
};
inline bool accepts(id,SEL) { return true; }
inline void mouseDown(id view,SEL,id) { send<void>(send(view,"window"),"makeFirstResponder:",view); }
inline void registerClasses() {
    if(!objc_getClass("ERWindow")) {
        Class window=objc_allocateClassPair(objc_getClass("NSWindow"),"ERWindow",0);
        class_addMethod(window,sel_registerName("canBecomeKeyWindow"),reinterpret_cast<IMP>(&accepts),"B@:");
        class_addMethod(window,sel_registerName("canBecomeMainWindow"),reinterpret_cast<IMP>(&accepts),"B@:");
        objc_registerClassPair(window);
        Class view=objc_allocateClassPair(objc_getClass("NSView"),"ERGameView",0);
        class_addMethod(view,sel_registerName("acceptsFirstResponder"),reinterpret_cast<IMP>(&accepts),"B@:");
        // acceptsFirstMouse: has an event argument, ignored by this implementation.
        class_addMethod(view,sel_registerName("acceptsFirstMouse:"),reinterpret_cast<IMP>(+[](id,SEL,id)->bool{return true;}),"B@:@");
        class_addMethod(view,sel_registerName("mouseDown:"),reinterpret_cast<IMP>(&mouseDown),"v@:@");
        objc_registerClassPair(view);
    }
}
}
struct MTLSize { NSUInteger width,height,depth; };
struct MTLOrigin { NSUInteger x,y,z; };
struct MTLRegion { MTLOrigin origin; MTLSize size; };
struct MTLClearColor { double red,green,blue,alpha; };
inline MTLSize MTLSizeMake(NSUInteger x,NSUInteger y,NSUInteger z) { return {x,y,z}; }
inline MTLOrigin MTLOriginMake(NSUInteger x,NSUInteger y,NSUInteger z) { return {x,y,z}; }
inline MTLRegion MTLRegionMake2D(NSUInteger x,NSUInteger y,NSUInteger w,NSUInteger h) { return {{x,y,0},{w,h,1}}; }
inline MTLClearColor MTLClearColorMake(double r,double g,double b,double a) { return {r,g,b,a}; }
inline constexpr NSUInteger MTLPixelFormatR8Unorm=10,MTLPixelFormatBGRA8Unorm=80,MTLPixelFormatRGBA16Float=115,MTLPixelFormatDepth32Float=252;
inline constexpr NSUInteger MTLStorageModePrivate=2,MTLResourceStorageModeShared=0,MTLResourceStorageModePrivate=32;
inline constexpr NSUInteger MTLTextureUsageShaderRead=1,MTLTextureUsageRenderTarget=4;
inline constexpr NSUInteger MTLLoadActionDontCare=0,MTLLoadActionClear=2,MTLStoreActionStore=1,MTLCompareFunctionLess=1,MTLPrimitiveTypeTriangle=3;
inline constexpr NSUInteger MTLCommandBufferStatusCompleted=4,MTLCommandBufferStatusError=5;
inline constexpr NSUInteger NSApplicationActivationPolicyRegular=0,NSBackingStoreBuffered=2,NSBitmapImageFileTypePNG=4;
inline constexpr NSUInteger NSWindowStyleMaskTitled=1,NSWindowStyleMaskClosable=2,NSWindowStyleMaskMiniaturizable=4,NSWindowStyleMaskResizable=8;
inline constexpr NSUInteger NSEventMaskAny=~NSUInteger(0),NSEventModifierFlagCommand=1ul<<20,NSEventModifierFlagShift=1ul<<17;
inline constexpr NSUInteger NSEventTypeLeftMouseDown=1,NSEventTypeLeftMouseUp=2,NSEventTypeMouseMoved=5,NSEventTypeLeftMouseDragged=6,NSEventTypeRightMouseDragged=7,NSEventTypeKeyDown=10,NSEventTypeKeyUp=11,NSEventTypeScrollWheel=22;
inline constexpr unichar NSUpArrowFunctionKey=0xf700,NSDownArrowFunctionKey=0xf701,NSLeftArrowFunctionKey=0xf702,NSRightArrowFunctionKey=0xf703,NSF8FunctionKey=0xf70b,NSF9FunctionKey=0xf70c;
