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
#include "constants.h"

/* Correct macOS virtual key code to RDP scan code mapping with extended flags */
static uint16 g_keycode_to_scancode[] = {
    /* 0x00 */ 0x1e, // kVK_ANSI_A
    /* 0x01 */ 0x1f, // kVK_ANSI_S
    /* 0x02 */ 0x20, // kVK_ANSI_D
    /* 0x03 */ 0x21, // kVK_ANSI_F
    /* 0x04 */ 0x23, // kVK_ANSI_H
    /* 0x05 */ 0x22, // kVK_ANSI_G
    /* 0x06 */ 0x2c, // kVK_ANSI_Z
    /* 0x07 */ 0x2d, // kVK_ANSI_X
    /* 0x08 */ 0x2e, // kVK_ANSI_C
    /* 0x09 */ 0x2f, // kVK_ANSI_V
    /* 0x0a */ 0x00, // Section key or other? unused
    /* 0x0b */ 0x30, // kVK_ANSI_B
    /* 0x0c */ 0x10, // kVK_ANSI_Q
    /* 0x0d */ 0x11, // kVK_ANSI_W
    /* 0x0e */ 0x12, // kVK_ANSI_E
    /* 0x0f */ 0x13, // kVK_ANSI_R
    /* 0x10 */ 0x15, // kVK_ANSI_Y
    /* 0x11 */ 0x14, // kVK_ANSI_T
    /* 0x12 */ 0x02, // kVK_ANSI_1
    /* 0x13 */ 0x03, // kVK_ANSI_2
    /* 0x14 */ 0x04, // kVK_ANSI_3
    /* 0x15 */ 0x05, // kVK_ANSI_4
    /* 0x16 */ 0x07, // kVK_ANSI_6
    /* 0x17 */ 0x06, // kVK_ANSI_5
    /* 0x18 */ 0x0d, // kVK_ANSI_Equal
    /* 0x19 */ 0x0a, // kVK_ANSI_9
    /* 0x1a */ 0x08, // kVK_ANSI_7
    /* 0x1b */ 0x0c, // kVK_ANSI_Minus
    /* 0x1c */ 0x09, // kVK_ANSI_8
    /* 0x1d */ 0x0b, // kVK_ANSI_0
    /* 0x1e */ 0x1b, // kVK_ANSI_RightBracket
    /* 0x1f */ 0x18, // kVK_ANSI_O
    /* 0x20 */ 0x16, // kVK_ANSI_U
    /* 0x21 */ 0x1a, // kVK_ANSI_LeftBracket
    /* 0x22 */ 0x17, // kVK_ANSI_I
    /* 0x23 */ 0x19, // kVK_ANSI_P
    /* 0x24 */ 0x1c, // kVK_Return
    /* 0x25 */ 0x26, // kVK_ANSI_L
    /* 0x26 */ 0x24, // kVK_ANSI_J
    /* 0x27 */ 0x28, // kVK_ANSI_Quote
    /* 0x28 */ 0x25, // kVK_ANSI_K
    /* 0x29 */ 0x27, // kVK_ANSI_Semicolon
    /* 0x2a */ 0x2b, // kVK_ANSI_Backslash
    /* 0x2b */ 0x33, // kVK_ANSI_Comma
    /* 0x2c */ 0x35, // kVK_ANSI_Slash
    /* 0x2d */ 0x31, // kVK_ANSI_N
    /* 0x2e */ 0x32, // kVK_ANSI_M
    /* 0x2f */ 0x34, // kVK_ANSI_Period
    /* 0x30 */ 0x0f, // kVK_Tab
    /* 0x31 */ 0x39, // kVK_Space
    /* 0x32 */ 0x29, // kVK_ANSI_Grave
    /* 0x33 */ 0x0e, // kVK_Delete (Backspace)
    /* 0x34 */ 0x00, // unused
    /* 0x35 */ 0x01, // kVK_Escape
    /* 0x36 */ 0x5c | KBD_FLAG_EXT, // Right Command (Right Windows Key)
    /* 0x37 */ 0x5b | KBD_FLAG_EXT, // kVK_Command (Left Command/Left Windows key)
    /* 0x38 */ 0x2a, // kVK_Shift (Left Shift)
    /* 0x39 */ 0x3a, // kVK_CapsLock
    /* 0x3a */ 0x38, // kVK_Option (Left Alt)
    /* 0x3b */ 0x1d, // kVK_Control (Left Control)
    /* 0x3c */ 0x36, // kVK_RightShift
    /* 0x3d */ 0x38 | KBD_FLAG_EXT, // kVK_RightOption (Right Alt)
    /* 0x3e */ 0x1d | KBD_FLAG_EXT, // kVK_RightControl
    /* 0x3f */ 0x00, // kVK_Function
    /* 0x40 */ 0x00, // kVK_F17
    /* 0x41 */ 0x53, // kVK_ANSI_KeypadDecimal
    /* 0x42 */ 0x00, // unused
    /* 0x43 */ 0x37, // kVK_ANSI_KeypadMultiply
    /* 0x44 */ 0x00, // unused
    /* 0x45 */ 0x4e, // kVK_ANSI_KeypadPlus
    /* 0x46 */ 0x00, // unused
    /* 0x47 */ 0x45, // kVK_ANSI_KeypadClear (Num Lock)
    /* 0x48 */ 0x00, // kVK_VolumeUp
    /* 0x49 */ 0x00, // kVK_VolumeDown
    /* 0x4a */ 0x00, // kVK_Mute
    /* 0x4b */ 0x35 | KBD_FLAG_EXT, // kVK_ANSI_KeypadDivide
    /* 0x4c */ 0x1c | KBD_FLAG_EXT, // kVK_ANSI_KeypadEnter
    /* 0x4d */ 0x00, // unused
    /* 0x4e */ 0x4a, // kVK_ANSI_KeypadMinus
    /* 0x4f */ 0x00, // kVK_F18
    /* 0x50 */ 0x00, // kVK_F19
    /* 0x51 */ 0x0d, // kVK_ANSI_KeypadEquals
    /* 0x52 */ 0x52, // kVK_ANSI_Keypad0
    /* 0x53 */ 0x4f, // kVK_ANSI_Keypad1
    /* 0x54 */ 0x50, // kVK_ANSI_Keypad2
    /* 0x55 */ 0x51, // kVK_ANSI_Keypad3
    /* 0x56 */ 0x4b, // kVK_ANSI_Keypad4
    /* 0x57 */ 0x4c, // kVK_ANSI_Keypad5
    /* 0x58 */ 0x4d, // kVK_ANSI_Keypad6
    /* 0x59 */ 0x47, // kVK_ANSI_Keypad7
    /* 0x5a */ 0x00, // kVK_F20
    /* 0x5b */ 0x48, // kVK_ANSI_Keypad8
    /* 0x5c */ 0x49, // kVK_ANSI_Keypad9
    /* 0x5d */ 0x00, // unused
    /* 0x5e */ 0x00, // unused
    /* 0x5f */ 0x00, // unused
    /* 0x60 */ 0x3f, // kVK_F5
    /* 0x61 */ 0x40, // kVK_F6
    /* 0x62 */ 0x41, // kVK_F7
    /* 0x63 */ 0x3d, // kVK_F3
    /* 0x64 */ 0x42, // kVK_F8
    /* 0x65 */ 0x43, // kVK_F9
    /* 0x66 */ 0x00, // unused
    /* 0x67 */ 0x57, // kVK_F11
    /* 0x68 */ 0x00, // unused
    /* 0x69 */ 0x37 | KBD_FLAG_EXT, // kVK_F13 (PrintScreen)
    /* 0x6a */ 0x00, // kVK_F16
    /* 0x6b */ 0x46, // kVK_F14 (ScrollLock)
    /* 0x6c */ 0x00, // unused
    /* 0x6d */ 0x44, // kVK_F10
    /* 0x6e */ 0x00, // unused
    /* 0x6f */ 0x58, // kVK_F12
    /* 0x70 */ 0x00, // unused
    /* 0x71 */ 0x45 | KBD_FLAG_EXT, // kVK_F15 (Pause/Break)
    /* 0x72 */ 0x52 | KBD_FLAG_EXT, // kVK_Help (Insert)
    /* 0x73 */ 0x47 | KBD_FLAG_EXT, // kVK_Home
    /* 0x74 */ 0x49 | KBD_FLAG_EXT, // kVK_PageUp
    /* 0x75 */ 0x53 | KBD_FLAG_EXT, // kVK_ForwardDelete (Delete)
    /* 0x76 */ 0x3e, // kVK_F4
    /* 0x77 */ 0x4f | KBD_FLAG_EXT, // kVK_End
    /* 0x78 */ 0x3c, // kVK_F2
    /* 0x79 */ 0x51 | KBD_FLAG_EXT, // kVK_PageDown
    /* 0x7a */ 0x3b, // kVK_F1
    /* 0x7b */ 0x4b | KBD_FLAG_EXT, // kVK_LeftArrow
    /* 0x7c */ 0x4d | KBD_FLAG_EXT, // kVK_RightArrow
    /* 0x7d */ 0x50 | KBD_FLAG_EXT, // kVK_DownArrow
    /* 0x7e */ 0x48 | KBD_FLAG_EXT, // kVK_UpArrow
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
    (void)keysym;
    (void)state;
    (void)ev_time;
    (void)pressed;
    return False; // Let normal key handling proceed
}

void
ensure_remote_modifiers(uint32 ev_time, key_translation tr)
{
    // Ensure modifier state is correct
    (void)ev_time;
    (void)tr;
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
    (void)state;
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

/* Convert macOS keycode to RDP scancode and extended flags */
uint16
mac_keycode_to_scancode(uint16 keycode)
{
    if (keycode < KEYCODE_TABLE_SIZE) {
        uint16 scancode_with_flags = g_keycode_to_scancode[keycode];
        if (scancode_with_flags != 0) {
            return scancode_with_flags;
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