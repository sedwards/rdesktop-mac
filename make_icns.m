#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

int main(int argc, char *argv[]) {
    @autoreleasepool {
        if (argc < 3) {
            fprintf(stderr, "Usage: make_icns <input.png> <output.icns>\n");
            return 1;
        }
        NSString *inputPath = [NSString stringWithUTF8String:argv[1]];
        NSString *outputPath = [NSString stringWithUTF8String:argv[2]];
        
        NSLog(@"Loading input image from %@", inputPath);
        NSImage *image = [[NSImage alloc] initWithContentsOfFile:inputPath];
        if (!image) {
            fprintf(stderr, "Error: Failed to load input image from %s\n", argv[1]);
            return 1;
        }
        
        NSString *iconsetDir = [[outputPath stringByDeletingPathExtension] stringByAppendingPathExtension:@"iconset"];
        NSLog(@"Creating iconset directory at %@", iconsetDir);
        NSFileManager *fm = [NSFileManager defaultManager];
        NSError *err = nil;
        if (![fm createDirectoryAtPath:iconsetDir withIntermediateDirectories:YES attributes:nil error:&err]) {
            NSLog(@"Failed to create iconset directory: %@", err);
            return 1;
        }
        
        int sizes[] = {16, 32, 128, 256, 512};
        for (int i = 0; i < 5; i++) {
            int size = sizes[i];
            for (int scale = 1; scale <= 2; scale++) {
                int targetSize = size * scale;
                
                NSBitmapImageRep *rep = [[NSBitmapImageRep alloc]
                    initWithBitmapDataPlanes:NULL
                                  pixelsWide:targetSize
                                  pixelsHigh:targetSize
                               bitsPerSample:8
                             samplesPerPixel:4
                                    hasAlpha:YES
                                    isPlanar:NO
                              colorSpaceName:NSCalibratedRGBColorSpace
                                 bytesPerRow:targetSize * 4
                                bitsPerPixel:32];
                
                if (!rep) {
                    fprintf(stderr, "Error: Failed to create NSBitmapImageRep for size %d\n", targetSize);
                    return 1;
                }
                
                [NSGraphicsContext saveGraphicsState];
                [NSGraphicsContext setCurrentContext:[NSGraphicsContext graphicsContextWithBitmapImageRep:rep]];
                
                [[NSGraphicsContext currentContext] setImageInterpolation:NSImageInterpolationHigh];
                [image drawInRect:NSMakeRect(0, 0, targetSize, targetSize)
                         fromRect:NSZeroRect
                        operation:NSCompositingOperationCopy
                         fraction:1.0];
                
                [NSGraphicsContext restoreGraphicsState];
                
                NSData *pngData = [rep representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
                if (!pngData) {
                    fprintf(stderr, "Error: Failed to generate PNG data for size %d\n", targetSize);
                    return 1;
                }
                
                NSString *filename;
                if (scale == 2) {
                    filename = [NSString stringWithFormat:@"icon_%dx%d@2x.png", size, size];
                } else {
                    filename = [NSString stringWithFormat:@"icon_%dx%d.png", size, size];
                }
                
                NSString *pngPath = [iconsetDir stringByAppendingPathComponent:filename];
                NSLog(@"Writing PNG to %@", pngPath);
                if (![pngData writeToFile:pngPath atomically:YES]) {
                    NSLog(@"Failed to write PNG to %@", pngPath);
                    return 1;
                }
            }
        }
        
        // Run iconutil to compile the iconset into icns
        NSLog(@"Running iconutil -c icns %@", iconsetDir);
        NSTask *task = [[NSTask alloc] init];
        [task setLaunchPath:@"/usr/bin/iconutil"];
        [task setArguments:@[@"-c", @"icns", iconsetDir]];
        [task launch];
        [task waitUntilExit];
        NSLog(@"iconutil finished with status %d", [task terminationStatus]);
        
        // Clean up the iconset directory
        NSLog(@"Cleaning up iconset directory %@", iconsetDir);
        if (![fm removeItemAtPath:iconsetDir error:&err]) {
            NSLog(@"Warning: Failed to clean up iconset directory: %@", err);
        }
        
        return 0;
    }
}
