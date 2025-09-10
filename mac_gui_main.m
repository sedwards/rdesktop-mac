/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   macOS GUI Application Main Entry Point
   Native macOS port
*/

#import <Cocoa/Cocoa.h>
#import "MacRDPConnectionGUI.h"
#include "rdesktop.h"

// External functions from rdesktop
extern int rdesktop_main(int argc, char *argv[]);

@interface MacRDPAppDelegate : NSObject <NSApplicationDelegate>
@property (strong) MacRDPConnectionGUI *connectionGUI;
@end

@implementation MacRDPAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    // Initialize connection GUI
    self.connectionGUI = [[MacRDPConnectionGUI alloc] init];
    [self.connectionGUI showConnectionWindow];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

@end

int main(int argc, char *argv[]) {
    @autoreleasepool {
        // Check if GUI mode is requested or if no arguments provided
        BOOL useGUI = NO;
        
        // Use GUI if no arguments or if --gui flag is present
        if (argc == 1) {
            useGUI = YES;
        } else {
            for (int i = 1; i < argc; i++) {
                if (strcmp(argv[i], "--gui") == 0) {
                    useGUI = YES;
                    break;
                }
            }
        }
        
        if (useGUI) {
            // Run macOS GUI application
            NSApplication *app = [NSApplication sharedApplication];
            [app setActivationPolicy:NSApplicationActivationPolicyRegular];
            
            MacRDPAppDelegate *delegate = [[MacRDPAppDelegate alloc] init];
            [app setDelegate:delegate];
            
[app run];
            return 0;
        } else {
            // Run traditional command-line rdesktop
            return rdesktop_main(argc, argv);
        }
    }
}

// Global storage for advanced options
static NSDictionary *g_advanced_options = nil;

void setAdvancedOptions(NSDictionary *options) {
    if (g_advanced_options) {
        [g_advanced_options release];
    }
    g_advanced_options = [options retain];
}

// Helper function to launch RDP connection from GUI
int launch_rdp_connection_from_gui(void) {
    // This function would be called by the GUI to start the actual RDP connection
    // It would use the global variables set by the GUI
    
    extern char g_hostname[16];
    extern char *g_username;
    extern int g_tcp_port_rdp;
    extern uint32 g_requested_session_width;
    extern uint32 g_requested_session_height;
    extern char g_keymapname[PATH_MAX];
    
    // Construct argc/argv equivalent for the connection - increased size for all options
    char *fake_argv[100];
    int fake_argc = 1;
    
    // Static buffers for arguments that need to persist
    static char geom_buf[32];
    static char port_buf[16];
    static char depth_buf[16];
    static char dpi_buf[16];
    static char exp_buf[16];
    
    fake_argv[0] = "rdesktop-mac";
    
    // Basic options
    if (g_username && strlen(g_username) > 0) {
        fake_argv[fake_argc++] = "-u";
        fake_argv[fake_argc++] = g_username;
    }
    
    // Add geometry
    snprintf(geom_buf, sizeof(geom_buf), "%dx%d", g_requested_session_width, g_requested_session_height);
    fake_argv[fake_argc++] = "-g";
    fake_argv[fake_argc++] = geom_buf;
    
    // Add keymap
    if (strlen(g_keymapname) > 0) {
        fake_argv[fake_argc++] = "-k";
        fake_argv[fake_argc++] = g_keymapname;
    }
    
    // Add port if not default
    if (g_tcp_port_rdp != 3389) {
        snprintf(port_buf, sizeof(port_buf), "%d", g_tcp_port_rdp);
        fake_argv[fake_argc++] = "-P";
        fake_argv[fake_argc++] = port_buf;
    }
    
    // Process advanced options if available
    if (g_advanced_options) {
        // Password
        NSString *password = [g_advanced_options objectForKey:@"password"];
        if (password && [password length] > 0) {
            fake_argv[fake_argc++] = "-p";
            fake_argv[fake_argc++] = (char *)[password UTF8String];
        }
        
        // Client hostname
        NSString *clientHostname = [g_advanced_options objectForKey:@"clientHostname"];
        if (clientHostname && [clientHostname length] > 0) {
            fake_argv[fake_argc++] = "-n";
            fake_argv[fake_argc++] = (char *)[clientHostname UTF8String];
        }
        
        // TLS Version
        NSString *tlsVersion = [g_advanced_options objectForKey:@"tlsVersion"];
        if (tlsVersion && [tlsVersion length] > 0) {
            fake_argv[fake_argc++] = "-V";
            fake_argv[fake_argc++] = (char *)[tlsVersion UTF8String];
        }
        
        // Color depth
        NSString *colorDepth = [g_advanced_options objectForKey:@"colorDepth"];
        if (colorDepth && [colorDepth length] > 0) {
            fake_argv[fake_argc++] = "-a";
            if ([colorDepth isEqualToString:@"8-bit"]) {
                fake_argv[fake_argc++] = "8";
            } else if ([colorDepth isEqualToString:@"15-bit"]) {
                fake_argv[fake_argc++] = "15";
            } else if ([colorDepth isEqualToString:@"16-bit"]) {
                fake_argv[fake_argc++] = "16";
            } else if ([colorDepth isEqualToString:@"24-bit"]) {
                fake_argv[fake_argc++] = "24";
            } else if ([colorDepth isEqualToString:@"32-bit"]) {
                fake_argv[fake_argc++] = "32";
            }
        }
        
        // Window title
        NSString *windowTitle = [g_advanced_options objectForKey:@"windowTitle"];
        if (windowTitle && [windowTitle length] > 0) {
            fake_argv[fake_argc++] = "-T";
            fake_argv[fake_argc++] = (char *)[windowTitle UTF8String];
        }
        
        // Experience level
        NSString *experience = [g_advanced_options objectForKey:@"experience"];
        if (experience && [experience length] > 0) {
            fake_argv[fake_argc++] = "-x";
            if ([experience isEqualToString:@"Modem (28.8k)"]) {
                fake_argv[fake_argc++] = "m";
            } else if ([experience isEqualToString:@"Broadband"]) {
                fake_argv[fake_argc++] = "b";
            } else if ([experience isEqualToString:@"LAN"]) {
                fake_argv[fake_argc++] = "l";
            }
        }
        
        // Shell/Application
        NSString *shell = [g_advanced_options objectForKey:@"shell"];
        if (shell && [shell length] > 0) {
            fake_argv[fake_argc++] = "-s";
            fake_argv[fake_argc++] = (char *)[shell UTF8String];
        }
        
        // Working directory
        NSString *workingDir = [g_advanced_options objectForKey:@"workingDir"];
        if (workingDir && [workingDir length] > 0) {
            fake_argv[fake_argc++] = "-c";
            fake_argv[fake_argc++] = (char *)[workingDir UTF8String];
        }
        
        // SeamlessRDP path
        NSString *seamlessPath = [g_advanced_options objectForKey:@"seamlessPath"];
        if (seamlessPath && [seamlessPath length] > 0) {
            fake_argv[fake_argc++] = "-A";
            fake_argv[fake_argc++] = (char *)[seamlessPath UTF8String];
        }
        
        // Local codepage
        NSString *codepage = [g_advanced_options objectForKey:@"codepage"];
        if (codepage && [codepage length] > 0) {
            fake_argv[fake_argc++] = "-L";
            fake_argv[fake_argc++] = (char *)[codepage UTF8String];
        }
        
        // Device redirection
        NSString *deviceRedirection = [g_advanced_options objectForKey:@"deviceRedirection"];
        if (deviceRedirection && [deviceRedirection length] > 0) {
            fake_argv[fake_argc++] = "-r";
            fake_argv[fake_argc++] = (char *)[deviceRedirection UTF8String];
        }
        
        // Boolean flags
        if ([g_advanced_options objectForKey:@"smartcard"]) {
            fake_argv[fake_argc++] = "-i";
        }
        if ([g_advanced_options objectForKey:@"disableEncryption"]) {
            fake_argv[fake_argc++] = "-e";
        }
        if ([g_advanced_options objectForKey:@"disableClientEncryption"]) {
            fake_argv[fake_argc++] = "-E";
        }
        if ([g_advanced_options objectForKey:@"fullscreen"]) {
            fake_argv[fake_argc++] = "-f";
        }
        if ([g_advanced_options objectForKey:@"forceBitmapUpdates"]) {
            fake_argv[fake_argc++] = "-b";
        }
        if ([g_advanced_options objectForKey:@"hideDecorations"]) {
            fake_argv[fake_argc++] = "-D";
        }
        if ([g_advanced_options objectForKey:@"compression"]) {
            fake_argv[fake_argc++] = "-z";
        }
        if ([g_advanced_options objectForKey:@"persistentCaching"]) {
            fake_argv[fake_argc++] = "-P";
        }
        if ([g_advanced_options objectForKey:@"backingStore"]) {
            fake_argv[fake_argc++] = "-B";
        }
        if ([g_advanced_options objectForKey:@"disableMotionEvents"]) {
            fake_argv[fake_argc++] = "-m";
        }
        if ([g_advanced_options objectForKey:@"localMouseCursor"]) {
            fake_argv[fake_argc++] = "-M";
        }
        if ([g_advanced_options objectForKey:@"numlockSync"]) {
            fake_argv[fake_argc++] = "-N";
        }
        if ([g_advanced_options objectForKey:@"disableRemoteCtrl"]) {
            fake_argv[fake_argc++] = "-t";
        }
        if ([g_advanced_options objectForKey:@"keepKeyBindings"]) {
            fake_argv[fake_argc++] = "-K";
        }
        if ([g_advanced_options objectForKey:@"console"]) {
            fake_argv[fake_argc++] = "-0";
        }
        if ([g_advanced_options objectForKey:@"verboseLogging"]) {
            fake_argv[fake_argc++] = "-v";
        }
        
        // RDP Version
        NSString *rdpVersion = [g_advanced_options objectForKey:@"rdpVersion"];
        if (rdpVersion && [rdpVersion isEqualToString:@"RDP 4"]) {
            fake_argv[fake_argc++] = "-4";
        } else {
            fake_argv[fake_argc++] = "-5"; // Default to RDP 5
        }
    }
    
    // Add hostname
    fake_argv[fake_argc++] = g_hostname;
    fake_argv[fake_argc] = NULL;
    
    // Call the main RDP connection function
    return rdesktop_main(fake_argc, fake_argv);
}