/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   User interface services - macOS Cocoa
   Native macOS port
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "rdesktop.h"
#import <Cocoa/Cocoa.h>
#import <CoreGraphics/CoreGraphics.h>
#include <assert.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <strings.h>

#ifdef __APPLE__
#include <sys/param.h>
#define HOST_NAME_MAX MAXHOSTNAMELEN
#endif

/* Forward declarations */
@interface RDPView : NSView
@property (nonatomic) CGContextRef backingStore;
@property (nonatomic) NSSize backingStoreSize;
- (void)setupBackingStore:(NSSize)size;
@end

@interface RDPWindow : NSWindow <NSWindowDelegate>
@property (nonatomic, strong) RDPView *rdpView;
@end

/* Global variables */
static NSApplication *g_app = nil;
static RDPWindow *g_window = nil;
static RDPView *g_view = nil;
static CGColorSpaceRef g_colorspace = NULL;

// RDP state
extern RD_BOOL g_rdpsnd;
extern RD_BOOL g_sendmotion;
extern RD_BOOL g_fullscreen;
extern RD_BOOL g_grab_keyboard;
extern RD_BOOL g_hide_decorations;
extern char g_title[];
extern uint32 g_rdp_shareid;

uint16 g_width = 800;
uint16 g_height = 600;
static uint32 g_server_depth = 32;
static RD_BOOL g_backstore = True;

/* Utility functions */
static void
mac_fatal(char *format, ...)
{
    va_list ap;
    char message[1024];
    
    va_start(ap, format);
    vsnprintf(message, sizeof(message), format, ap);
    va_end(ap);
    
    NSLog(@"Fatal error: %s", message);
    exit(1);
}

@implementation RDPView

- (void)setupBackingStore:(NSSize)size {
    if (self.backingStore) {
        CGContextRelease(self.backingStore);
    }
    
    self.backingStoreSize = size;
    
    CGContextRef context = CGBitmapContextCreate(
        NULL,                               // data
        size.width,                         // width
        size.height,                        // height
        8,                                  // bitsPerComponent
        size.width * 4,                     // bytesPerRow
        g_colorspace,                       // colorspace
        kCGImageAlphaPremultipliedLast      // bitmapInfo
    );
    
    if (!context) {
        mac_fatal("Failed to create backing store context");
    }
    
    self.backingStore = context;
    
    // Clear to black
    CGContextSetRGBFillColor(context, 0, 0, 0, 1);
    CGContextFillRect(context, CGRectMake(0, 0, size.width, size.height));
}

- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) {
        [self setupBackingStore:frameRect.size];
    }
    return self;
}

- (void)dealloc {
    if (self.backingStore) {
        CGContextRelease(self.backingStore);
    }
    [super dealloc];
}

- (void)drawRect:(NSRect)dirtyRect {
    CGContextRef context = [[NSGraphicsContext currentContext] CGContext];
    
    if (self.backingStore) {
        CGImageRef image = CGBitmapContextCreateImage(self.backingStore);
        CGContextDrawImage(context, self.bounds, image);
        CGImageRelease(image);
    }
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)keyDown:(NSEvent *)event {
    // Convert NSEvent to RDP key event
    uint16 keycode = [event keyCode];
    rdp_send_scancode([event timestamp] * 1000, RDP_KEYPRESS, keycode);
}

- (void)keyUp:(NSEvent *)event {
    uint16 keycode = [event keyCode];
    rdp_send_scancode([event timestamp] * 1000, RDP_KEYRELEASE, keycode);
}

- (void)mouseDown:(NSEvent *)event {
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    rdp_send_input([event timestamp] * 1000, RDP_INPUT_MOUSE, 
                   MOUSE_FLAG_BUTTON1 | MOUSE_FLAG_DOWN, point.x, point.y);
}

- (void)mouseUp:(NSEvent *)event {
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    rdp_send_input([event timestamp] * 1000, RDP_INPUT_MOUSE, 
                   MOUSE_FLAG_BUTTON1, point.x, point.y);
}

- (void)rightMouseDown:(NSEvent *)event {
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    rdp_send_input([event timestamp] * 1000, RDP_INPUT_MOUSE, 
                   MOUSE_FLAG_BUTTON2 | MOUSE_FLAG_DOWN, point.x, point.y);
}

- (void)rightMouseUp:(NSEvent *)event {
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    rdp_send_input([event timestamp] * 1000, RDP_INPUT_MOUSE, 
                   MOUSE_FLAG_BUTTON2, point.x, point.y);
}

- (void)mouseMoved:(NSEvent *)event {
    if (g_sendmotion) {
        NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
        rdp_send_input([event timestamp] * 1000, RDP_INPUT_MOUSE, 
                       MOUSE_FLAG_MOVE, point.x, point.y);
    }
}

@end

@implementation RDPWindow

- (instancetype)initWithContentRect:(NSRect)contentRect {
    NSUInteger styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | 
                          NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;
    
    self = [super initWithContentRect:contentRect
                            styleMask:styleMask
                              backing:NSBackingStoreBuffered
                                defer:NO];
    
    if (self) {
        [self setDelegate:self];
        
        self.rdpView = [[RDPView alloc] initWithFrame:contentRect];
        [self setContentView:self.rdpView];
        
        [self makeFirstResponder:self.rdpView];
    }
    
    return self;
}

- (void)windowWillClose:(NSNotification *)notification {
    [NSApp terminate:self];
}

@end

/* UI Interface Implementation */

RD_BOOL
ui_init(void)
{
    g_app = [NSApplication sharedApplication];
    [g_app setActivationPolicy:NSApplicationActivationPolicyRegular];
    
    g_colorspace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
    if (!g_colorspace) {
        mac_fatal("Failed to create color space");
        return False;
    }
    
    return True;
}

void
ui_get_screen_size(uint32 * width, uint32 * height)
{
    NSScreen *mainScreen = [NSScreen mainScreen];
    NSRect screenRect = [mainScreen frame];
    *width = (uint32)screenRect.size.width;
    *height = (uint32)screenRect.size.height;
}

void
ui_get_screen_size_from_percentage(uint32 pw, uint32 ph, uint32 * width, uint32 * height)
{
    uint32 sw, sh;
    ui_get_screen_size(&sw, &sh);
    *width = sw * pw / 100;
    *height = sh * ph / 100;
}

void
ui_get_workarea_size(uint32 * width, uint32 * height)
{
    NSScreen *mainScreen = [NSScreen mainScreen];
    NSRect visibleFrame = [mainScreen visibleFrame];
    *width = (uint32)visibleFrame.size.width;
    *height = (uint32)visibleFrame.size.height;
}

void
ui_deinit(void)
{
    if (g_colorspace) {
        CGColorSpaceRelease(g_colorspace);
        g_colorspace = NULL;
    }
}

void
ui_update_window_sizehints(uint32 width, uint32 height)
{
    if (g_window) {
        NSSize minSize = NSMakeSize(width, height);
        [g_window setContentMinSize:minSize];
    }
}

RD_BOOL
ui_create_window(uint32 width, uint32 height)
{
    g_width = width;
    g_height = height;
    
    NSRect contentRect = NSMakeRect(0, 0, width, height);
    g_window = [[RDPWindow alloc] initWithContentRect:contentRect];
    
    if (!g_window) {
        mac_fatal("Failed to create window");
        return False;
    }
    
    g_view = g_window.rdpView;
    
    [g_window setTitle:[NSString stringWithUTF8String:g_title]];
    [g_window center];
    [g_window makeKeyAndOrderFront:nil];
    
    [g_app activateIgnoringOtherApps:YES];
    
    return True;
}

void
ui_resize_window(uint32 width, uint32 height)
{
    if (g_window && g_view) {
        g_width = width;
        g_height = height;
        
        NSSize newSize = NSMakeSize(width, height);
        [g_window setContentSize:newSize];
        [g_view setupBackingStore:newSize];
    }
}

RD_BOOL
ui_have_window(void)
{
    return g_window != nil;
}

void
ui_destroy_window(void)
{
    if (g_window) {
        [g_window close];
        g_window = nil;
        g_view = nil;
    }
}

void
ui_select(int rdp_socket)
{
    fd_set rfds;
    struct timeval tv;
    int retval;
    
    FD_ZERO(&rfds);
    FD_SET(rdp_socket, &rfds);
    
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    
    retval = select(rdp_socket + 1, &rfds, NULL, NULL, &tv);
    (void)retval; // Suppress unused variable warning
    
    // Process Cocoa events
    NSEvent *event = [g_app nextEventMatchingMask:NSEventMaskAny
                                        untilDate:[NSDate distantPast]
                                           inMode:NSDefaultRunLoopMode
                                          dequeue:YES];
    if (event) {
        [g_app sendEvent:event];
    }
    
    // Note: return value ignored in void function
}

void
ui_move_pointer(int x, int y)
{
    (void)x; (void)y;
    // Move cursor - not implemented for now
}

RD_HBITMAP
ui_create_bitmap(int width, int height, uint8 * data)
{
    CGContextRef context = CGBitmapContextCreate(
        data,                               // data
        width,                              // width  
        height,                             // height
        8,                                  // bitsPerComponent
        width * 4,                          // bytesPerRow
        g_colorspace,                       // colorspace
        kCGImageAlphaNoneSkipLast           // bitmapInfo
    );
    
    return (RD_HBITMAP)context;
}

void
ui_paint_bitmap(int x, int y, int cx, int cy, int width, int height, uint8 * data)
{
    if (!g_view || !g_view.backingStore) return;
    
    CGContextRef context = g_view.backingStore;
    
    CGDataProviderRef dataProvider = CGDataProviderCreateWithData(NULL, data, width * height * 4, NULL);
    CGImageRef image = CGImageCreate(
        width,                              // width
        height,                             // height
        8,                                  // bitsPerComponent
        32,                                 // bitsPerPixel
        width * 4,                          // bytesPerRow
        g_colorspace,                       // colorspace
        (CGBitmapInfo)kCGImageAlphaNoneSkipLast,          // bitmapInfo
        dataProvider,                       // dataProvider
        NULL,                               // decode
        false,                              // shouldInterpolate
        kCGRenderingIntentDefault           // intent
    );
    
    CGRect destRect = CGRectMake(x, g_view.backingStoreSize.height - y - cy, cx, cy);
    CGRect sourceRect = CGRectMake(0, 0, width, height);
    
    CGImageRef croppedImage = CGImageCreateWithImageInRect(image, sourceRect);
    CGContextDrawImage(context, destRect, croppedImage);
    
    CGImageRelease(croppedImage);
    CGImageRelease(image);
    CGDataProviderRelease(dataProvider);
    
    // Mark view as needing display
    dispatch_async(dispatch_get_main_queue(), ^{
        [g_view setNeedsDisplayInRect:NSMakeRect(x, y, cx, cy)];
    });
}

void
ui_destroy_bitmap(RD_HBITMAP bmp)
{
    if (bmp) {
        CGContextRelease((CGContextRef)bmp);
    }
}

/* Simplified stubs for functions we don't immediately need */
RD_HGLYPH ui_create_glyph(int width, int height, uint8 * data) { 
    (void)width; (void)height; (void)data;
    return NULL; 
}
void ui_destroy_glyph(RD_HGLYPH glyph) { (void)glyph; }
RD_HCURSOR ui_create_cursor(unsigned int x, unsigned int y, uint32 width, uint32 height, uint8 * andmask, uint8 * xormask, int bpp) { 
    (void)x; (void)y; (void)width; (void)height; (void)andmask; (void)xormask; (void)bpp;
    return NULL; 
}
void ui_set_cursor(RD_HCURSOR cursor) { (void)cursor; }
void ui_destroy_cursor(RD_HCURSOR cursor) { (void)cursor; }
void ui_set_null_cursor(void) {}
void ui_set_standard_cursor(void) {}
RD_HCOLOURMAP ui_create_colourmap(COLOURMAP * colours) { (void)colours; return NULL; }
void ui_destroy_colourmap(RD_HCOLOURMAP map) { (void)map; }
void ui_set_colourmap(RD_HCOLOURMAP map) { (void)map; }
void ui_set_clip(int x, int y, int cx, int cy) { (void)x; (void)y; (void)cx; (void)cy; }
void ui_reset_clip(void) {}
void ui_bell(void) { NSBeep(); }

/* Drawing operations - basic implementations */
void ui_destblt(uint8 opcode, int x, int y, int cx, int cy) {}
void ui_patblt(uint8 opcode, int x, int y, int cx, int cy, BRUSH * brush, uint32 bgcolor, uint32 fgcolor) {}
void ui_screenblt(uint8 opcode, int x, int y, int cx, int cy, int srcx, int srcy) {}
void ui_memblt(uint8 opcode, int x, int y, int cx, int cy, RD_HBITMAP src, int srcx, int srcy) {}
void ui_triblt(uint8 opcode, int x, int y, int cx, int cy, RD_HBITMAP src, int srcx, int srcy, BRUSH * brush, uint32 bgcolor, uint32 fgcolor) {}
void ui_line(uint8 opcode, int startx, int starty, int endx, int endy, PEN * pen) {}
void ui_rect(int x, int y, int cx, int cy, uint32 colour) {}
void ui_polygon(uint8 opcode, uint8 fillmode, RD_POINT * point, int npoints, BRUSH * brush, uint32 bgcolor, uint32 fgcolor) {}
void ui_polyline(uint8 opcode, RD_POINT * points, int npoints, PEN * pen) {}
void ui_ellipse(uint8 opcode, uint8 fillmode, int x, int y, int cx, int cy, BRUSH * brush, uint32 bgcolor, uint32 fgcolor) {}
void ui_draw_glyph(int mixmode, int x, int y, int cx, int cy, RD_HGLYPH glyph, int srcx, int srcy, uint32 bgcolour, uint32 fgcolour) {}
void ui_draw_text(uint8 font, uint8 flags, uint8 opcode, int mixmode, int x, int y, int clipx, int clipy, int clipcx, int clipcy, int boxx, int boxy, int boxcx, int boxcy, BRUSH * brush, uint32 bgcolor, uint32 fgcolor, uint8 * text, uint8 length) {}
void ui_desktop_save(uint32 offset, int x, int y, int cx, int cy) {}
void ui_desktop_restore(uint32 offset, int x, int y, int cx, int cy) {}
void ui_begin_update(void) {}
void ui_end_update(void) {}

/* Seamless mode stubs */
void ui_seamless_begin(RD_BOOL hidden) {}
void ui_seamless_end(void) {}
void ui_seamless_hide_desktop(void) {}
void ui_seamless_unhide_desktop(void) {}
void ui_seamless_toggle(void) {}
void ui_seamless_create_window(unsigned long id, unsigned long group, unsigned long parent, unsigned long flags) {}
void ui_seamless_destroy_window(unsigned long id, unsigned long flags) {}
void ui_seamless_destroy_group(unsigned long id, unsigned long flags) {}
void ui_seamless_seticon(unsigned long id, const char *format, int width, int height, int chunk, const char *data, size_t chunk_len) {}
void ui_seamless_delicon(unsigned long id, const char *format, int width, int height) {}
void ui_seamless_move_window(unsigned long id, int x, int y, int width, int height, unsigned long flags) {}
void ui_seamless_restack_window(unsigned long id, unsigned long behind, unsigned long flags) {}
void ui_seamless_settitle(unsigned long id, const char *title, unsigned long flags) {}
void ui_seamless_setstate(unsigned long id, unsigned int state, unsigned long flags) {}
void ui_seamless_syncbegin(unsigned long flags) {}
void ui_seamless_ack(unsigned int serial) {}

/* Global variables for dynamic session management */
RD_BOOL g_dynamic_session_resize = False;
uint32 g_wait_for_deactivate_ts = 0;