// Optional macOS tooling: clang tools/media.m -o /tmp/eternal-media -framework AppKit -framework AVFoundation -framework CoreMedia -Wno-deprecated-declarations -Wno-incompatible-pointer-types
#import <AppKit/AppKit.h>
#import <AVFoundation/AVFoundation.h>
int main(int argc, const char **argv) { @autoreleasepool {
    if (argc == 5 && !strcmp(argv[1], "compare")) {
        NSImage *left = [[NSImage alloc] initWithContentsOfFile:@(argv[2])];
        NSImage *right = [[NSImage alloc] initWithContentsOfFile:@(argv[3])];
        if (!left || !right) return 1;
        int height = 700, lw = (int)(left.size.width / left.size.height * height), rw = (int)(right.size.width / right.size.height * height);
        NSBitmapImageRep *rep = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL pixelsWide:lw+rw+24 pixelsHigh:height+32 bitsPerSample:8 samplesPerPixel:4 hasAlpha:YES isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace bytesPerRow:0 bitsPerPixel:0];
        [NSGraphicsContext saveGraphicsState];
        [NSGraphicsContext setCurrentContext:[NSGraphicsContext graphicsContextWithBitmapImageRep:rep]];
        [[NSColor colorWithRed:11/255.0 green:11/255.0 blue:26/255.0 alpha:1] setFill]; NSRectFill(NSMakeRect(0,0,lw+rw+24,height+32));
        [left drawInRect:NSMakeRect(0,0,lw,height)]; [right drawInRect:NSMakeRect(lw+24,0,rw,height)];
        NSDictionary *attr=@{NSFontAttributeName:[NSFont fontWithName:@"Menlo" size:14],NSForegroundColorAttributeName:NSColor.whiteColor};
        [@"VIDEO REFERENCE" drawAtPoint:NSMakePoint(8,height+8) withAttributes:attr];
        [@"ETERNALRPG / ACTUAL FRAMEBUFFER" drawAtPoint:NSMakePoint(lw+32,height+8) withAttributes:attr];
        [NSGraphicsContext restoreGraphicsState];
        [[rep representationUsingType:NSBitmapImageFileTypePNG properties:@{}] writeToFile:@(argv[4]) atomically:YES]; return 0;
    }
    if (argc == 4 && !strcmp(argv[1], "render")) {
        NSData *data = [NSData dataWithContentsOfFile:@(argv[2])];
        NSDictionary *frame = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
        int w = [frame[@"width"] intValue], h = [frame[@"height"] intValue];
        NSBitmapImageRep *rep = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL pixelsWide:w*8 pixelsHigh:h*14 bitsPerSample:8 samplesPerPixel:4 hasAlpha:YES isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace bytesPerRow:0 bitsPerPixel:0];
        [NSGraphicsContext saveGraphicsState];
        [NSGraphicsContext setCurrentContext:[NSGraphicsContext graphicsContextWithBitmapImageRep:rep]];
        [[NSColor colorWithRed:11/255.0 green:11/255.0 blue:26/255.0 alpha:1] setFill];
        NSRectFill(NSMakeRect(0,0,w*8,h*14));
        NSFont *font = [NSFont fontWithName:@"Menlo" size:12];
        int i=0;
        for (NSArray *cell in frame[@"cells"]) {
            NSColor *color = [NSColor colorWithRed:[cell[1] doubleValue]/255 green:[cell[2] doubleValue]/255 blue:[cell[3] doubleValue]/255 alpha:1];
            [cell[0] drawAtPoint:NSMakePoint((i%w)*8,(h-1-i/w)*14) withAttributes:@{NSFontAttributeName:font,NSForegroundColorAttributeName:color}];
            ++i;
        }
        [NSGraphicsContext restoreGraphicsState];
        [[rep representationUsingType:NSBitmapImageFileTypePNG properties:@{}] writeToFile:@(argv[3]) atomically:YES];
        return 0;
    }
    NSString *root = argc>1 ? @(argv[1]) : @"/Users/cristopher/Downloads";
    NSArray *names = @[@"WhatsApp Video 2026-10-11 at 08.32.25.mp4", @"WhatsApp Video 2026-10-11 at 08.32.26.mp4", @"WhatsApp Video 2026-10-11 at 08.32.47.mp4", @"WhatsApp Video 2026-10-11 at 08.32.47 (1).mp4", @"WhatsApp Video 2026-10-11 at 08.32.47 (2).mp4"];
    NSArray *labels = @[@"spell",@"forest",@"grass",@"dragon",@"water"];
    for (int i=0;i<5;++i) {
        AVURLAsset *asset = [AVURLAsset URLAssetWithURL:[NSURL fileURLWithPath:[root stringByAppendingPathComponent:names[i]]] options:nil];
        double duration = CMTimeGetSeconds(asset.duration);
        AVAssetImageGenerator *gen = [AVAssetImageGenerator assetImageGeneratorWithAsset:asset];
        gen.appliesPreferredTrackTransform=YES;
        printf("%s: %.2f seconds\n", [labels[i] UTF8String], duration);
        for (int j=0;j<3;++j) {
            NSError *error=nil;
            CGImageRef image = [gen copyCGImageAtTime:CMTimeMakeWithSeconds(duration*(0.15+j*0.3),600) actualTime:NULL error:&error];
            if (!image) { NSLog(@"%@",error); return 1; }
            NSBitmapImageRep *rep=[[NSBitmapImageRep alloc] initWithCGImage:image];
            NSString *path=[NSString stringWithFormat:@"docs/reference/%@-%d.png",labels[i],j];
            [[rep representationUsingType:NSBitmapImageFileTypePNG properties:@{}] writeToFile:path atomically:YES];
            CGImageRelease(image);
        }
    }
    return 0;
} }
