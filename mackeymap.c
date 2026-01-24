/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop RDP_Protocol client.
   Keyboard mapping - macOS native
   Native macOS port
*/

#include <Carbon/Carbon.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include "rdesktop.h"
#include "scancodes.h"

/* Simple macOS key code to RDP scan code mapping */
static uint8 g_keycode_to_scancode[] = {
    // Based on macOS virtual key codes
    0x1e, // 0x00 - kVK_ANSI_A
    0x30, // 0x01 - kVK_ANSI_S
    0x2e, // 0x02 - kVK_ANSI_D
    0x20, // 0x03 - kVK_ANSI_F
    0x23, // 0x04 - kVK_ANSI_H
    0x22, // 0x05 - kVK_ANSI_G
    0x2c, // 0x06 - kVK_ANSI_Z
    0x2d, // 0x07 - kVK_ANSI_X
    0x2f, // 0x08 - kVK_ANSI_C
    0x2f, // 0x09 - kVK_ANSI_V
    0x00, // 0x0A - unused
    0x32, // 0x0B - kVK_ANSI_B
    0x10, // 0x0C - kVK_ANSI_Q
    0x11, // 0x0D - kVK_ANSI_W
    0x12, // 0x0E - kVK_ANSI_E
    0x13, // 0x0F - kVK_ANSI_R
    0x15, // 0x10 - kVK_ANSI_Y
    0x14, // 0x11 - kVK_ANSI_T
    0x02, // 0x12 - kVK_ANSI_1
    0x03, // 0x13 - kVK_ANSI_2
    0x04, // 0x14 - kVK_ANSI_3
    0x05, // 0x15 - kVK_ANSI_4
    0x07, // 0x16 - kVK_ANSI_6
    0x06, // 0x17 - kVK_ANSI_5
    0x0d, // 0x18 - kVK_ANSI_Equal
    0x0a, // 0x19 - kVK_ANSI_9
    0x08, // 0x1A - kVK_ANSI_7
    0x0c, // 0x1B - kVK_ANSI_Minus
    0x09, // 0x1C - kVK_ANSI_8
    0x0b, // 0x1D - kVK_ANSI_0
    0x1b, // 0x1E - kVK_ANSI_RightBracket
    0x18, // 0x1F - kVK_ANSI_O
    0x16, // 0x20 - kVK_ANSI_U
    0x1a, // 0x21 - kVK_ANSI_LeftBracket
    0x17, // 0x22 - kVK_ANSI_I
    0x19, // 0x23 - kVK_ANSI_P
    0x1c, // 0x24 - kVK_Return
    0x26, // 0x25 - kVK_ANSI_L
    0x24, // 0x26 - kVK_ANSI_J
    0x28, // 0x27 - kVK_ANSI_Quote
    0x25, // 0x28 - kVK_ANSI_K
    0x27, // 0x29 - kVK_ANSI_Semicolon
    0x2b, // 0x2A - kVK_ANSI_Backslash
    0x33, // 0x2B - kVK_ANSI_Comma
    0x35, // 0x2C - kVK_ANSI_Slash
    0x31, // 0x2D - kVK_ANSI_N
    0x32, // 0x2E - kVK_ANSI_M
    0x34, // 0x2F - kVK_ANSI_Period
    0x0f, // 0x30 - kVK_Tab
    0x39, // 0x31 - kVK_Space
    0x29, // 0x32 - kVK_ANSI_Grave
    0x0e, // 0x33 - kVK_Delete (Backspace)
    0x00, // 0x34 - unused
    0x01, // 0x35 - kVK_Escape
    0x00, // 0x36 - Right Command
    0x5b, // 0x37 - kVK_Command (Left Command/Windows key)
    0x2a, // 0x38 - kVK_Shift (Left Shift)
    0x3a, // 0x39 - kVK_CapsLock
    0x38, // 0x3A - kVK_Option (Left Alt)
    0x1d, // 0x3B - kVK_Control (Left Control)
    0x36, // 0x3C - kVK_RightShift
    0x38, // 0x3D - kVK_RightOption (Right Alt)
    0x1d, // 0x3E - kVK_RightControl
    0x00, // 0x3F - kVK_Function
    0x00, // 0x40 - kVK_F17
    0x53, // 0x41 - kVK_ANSI_KeypadDecimal
    0x00, // 0x42 - unused
    0x37, // 0x43 - kVK_ANSI_KeypadMultiply
    0x00, // 0x44 - unused
    0x4e, // 0x45 - kVK_ANSI_KeypadPlus
    0x00, // 0x46 - unused
    0x45, // 0x47 - kVK_ANSI_KeypadClear (Num Lock)
    0x00, // 0x48 - kVK_VolumeUp
    0x00, // 0x49 - kVK_VolumeDown
    0x00, // 0x4A - kVK_Mute
    0x35, // 0x4B - kVK_ANSI_KeypadDivide
    0x1c, // 0x4C - kVK_ANSI_KeypadEnter
    0x00, // 0x4D - unused
    0x4a, // 0x4E - kVK_ANSI_KeypadMinus
    0x00, // 0x4F - kVK_F18
    0x00, // 0x50 - kVK_F19
    0x0d, // 0x51 - kVK_ANSI_KeypadEquals
    0x52, // 0x52 - kVK_ANSI_Keypad0
    0x4f, // 0x53 - kVK_ANSI_Keypad1
    0x50, // 0x54 - kVK_ANSI_Keypad2
    0x51, // 0x55 - kVK_ANSI_Keypad3
    0x4b, // 0x56 - kVK_ANSI_Keypad4
    0x4c, // 0x57 - kVK_ANSI_Keypad5
    0x4d, // 0x58 - kVK_ANSI_Keypad6
    0x47, // 0x59 - kVK_ANSI_Keypad7
    0x00, // 0x5A - kVK_F20
    0x48, // 0x5B - kVK_ANSI_Keypad8
    0x49, // 0x5C - kVK_ANSI_Keypad9
    0x00, // 0x5D - unused
    0x00, // 0x5E - unused
    0x00, // 0x5F - unused
    0x3f, // 0x60 - kVK_F5
    0x40, // 0x61 - kVK_F6
    0x41, // 0x62 - kVK_F7
    0x3d, // 0x63 - kVK_F3
    0x42, // 0x64 - kVK_F8
    0x43, // 0x65 - kVK_F9
    0x00, // 0x66 - unused
    0x57, // 0x67 - kVK_F11
    0x00, // 0x68 - unused
    0x37, // 0x69 - kVK_F13
    0x00, // 0x6A - kVK_F16
    0x54, // 0x6B - kVK_F14
    0x00, // 0x6C - unused
    0x44, // 0x6D - kVK_F10
    0x00, // 0x6E - unused
    0x58, // 0x6F - kVK_F12
    0x00, // 0x70 - unused
    0x00, // 0x71 - kVK_F15
    0x52, // 0x72 - kVK_Help (Insert)
    0x47, // 0x73 - kVK_Home
    0x49, // 0x74 - kVK_PageUp
    0x53, // 0x75 - kVK_ForwardDelete (Delete)
    0x3e, // 0x76 - kVK_F4
    0x4f, // 0x77 - kVK_End
    0x3c, // 0x78 - kVK_F2
    0x51, // 0x79 - kVK_PageDown
    0x3b, // 0x7A - kVK_F1
    0x4b, // 0x7B - kVK_LeftArrow
    0x4d, // 0x7C - kVK_RightArrow
    0x50, // 0x7D - kVK_DownArrow
    0x48, // 0x7E - kVK_UpArrow
};

#define KEYCODE_TABLE_SIZE (sizeof(g_keycode_to_scancode) / sizeof(g_keycode_to_scancode[0]))

/* Stub functions for compatibility */
void
xkeymap_init(void)
{
    // Initialize keymap - nothing needed for macOS
}

RD_BOOL
handle_special_keys(uint32 keysym, unsigned int state, uint32 ev_time, RD_BOOL pressed)
{
    // Handle special key combinations
    return False; // Let normal key handling proceed
}

void
ensure_remote_modifiers(uint32 ev_time, key_translation tr)
{
    // Ensure modifier state is correct
}

unsigned int
read_keyboard_state(void)
{
    // Read current modifier state
    return 0;
}

uint16
ui_get_numlock_state(unsigned int state)
{
    // Get numlock state
    return 0;
}

void
reset_modifier_keys(void)
{
    // Reset all modifier keys
}

void
rdp_send_scancode(uint32 time, uint16 flags, uint8 scancode)
{
    // Send scancode to RDP server
    rdp_send_input(time, RDP_INPUT_SCANCODE, flags, scancode, 0);
}

/* Convert macOS keycode to RDP scancode */
uint8
mac_keycode_to_scancode(uint16 keycode)
{
    if (keycode < KEYCODE_TABLE_SIZE) {
        uint8 scancode = g_keycode_to_scancode[keycode];
        if (scancode != 0) {
            return scancode;
        }
    }
    
    // Default fallback
    return 0x01; // Escape key as safe fallback
}

RD_BOOL
xkeymap_from_locale(const char *locale)
{
    // Stub implementation for macOS
    // The locale parameter is ignored as we use system keymap
    (void)locale;
    return True;
}