/* Copyright 2020 "Leo" Dmitry Kuznetsov
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
       http://www.apache.org/licenses/LICENSE-2.0
   Unless required by applicable law or agreed to in writing, software distributed
   under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied. See the License for the specific
   language governing permissions and limitations under the License.
*/
#include "c.h"
#include "ui.h"
#include <android/input.h>
#include <android/keycodes.h>

begin_c

typedef struct droid_key_text_string_s {
    char text[128];
} droid_key_text_string_t;

droid_key_text_string_t  droid_keys_text(int flags, int ch) {
    char state[128] = {};
    if (flags & KEYBOARD_KEY_PRESSED)  { strncat(state, "PRESSED|",  countof(state)); }
    if (flags & KEYBOARD_KEY_RELEASED) { strncat(state, "RELEASED|", countof(state)); }
    if (flags & KEYBOARD_KEY_REPEAT)   { strncat(state, "REPEAT|",   countof(state)); }
    if (flags & KEYBOARD_SHIFT)        { strncat(state, "SHIFT|",    countof(state)); }
    if (flags & KEYBOARD_ALT)          { strncat(state, "ALT|",      countof(state)); }
    if (flags & KEYBOARD_CTRL)         { strncat(state, "CTRL|",     countof(state)); }
    if (flags & KEYBOARD_SYM)          { strncat(state, "SYM|",      countof(state)); }
    if (flags & KEYBOARD_FN)           { strncat(state, "FN|",       countof(state)); }
    if (flags & KEYBOARD_NUMPAD)       { strncat(state, "NUMPAD|",   countof(state)); }
    if (flags & KEYBOARD_NUMLOCK)      { strncat(state, "NUMLOCK|",  countof(state)); }
    if (flags & KEYBOARD_CAPSLOCK)     { strncat(state, "CAPSLOCK|", countof(state)); }
    const char* mnemonic = null;
    switch (ch) {
        case KEY_CODE_ENTER    : mnemonic = "ENTER";     break;
        case KEY_CODE_LEFT     : mnemonic = "LEFT";      break;
        case KEY_CODE_RIGHT    : mnemonic = "RIGHT";     break;
        case KEY_CODE_UP       : mnemonic = "UP";        break;
        case KEY_CODE_DOWN     : mnemonic = "DOWN";      break;
        case KEY_CODE_BACKSPACE: mnemonic = "BACKSPACE"; break;
        case KEY_CODE_PAGE_UP  : mnemonic = "PAGE_UP";   break;
        case KEY_CODE_PAGE_DOWN: mnemonic = "PAGE_DOWN"; break;
        case KEY_CODE_HOME     : mnemonic = "HOME";      break;
        case KEY_CODE_END      : mnemonic = "END";       break;
        case KEY_CODE_BACK     : mnemonic = "BACK";      break;
        case KEY_CODE_ESC      : mnemonic = "ESC";       break;
        case KEY_CODE_DEL      : mnemonic = "DEL";       break;
        case KEY_CODE_INS      : mnemonic = "INS";       break;
        case KEY_CODE_CENTER   : mnemonic = "CENTER";    break;
        case KEY_CODE_ALT      : mnemonic = "ALT";       break;
        case KEY_CODE_CTRL     : mnemonic = "CTRL";      break;
        case KEY_CODE_SHIFT    : mnemonic = "SHIFT";     break;
        case KEY_CODE_F1       : mnemonic = "F1";        break;
        case KEY_CODE_F2       : mnemonic = "F2";        break;
        case KEY_CODE_F3       : mnemonic = "F3";        break;
        case KEY_CODE_F4       : mnemonic = "F4";        break;
        case KEY_CODE_F5       : mnemonic = "F5";        break;
        case KEY_CODE_F6       : mnemonic = "F6";        break;
        case KEY_CODE_F7       : mnemonic = "F7";        break;
        case KEY_CODE_F8       : mnemonic = "F8";        break;
        case KEY_CODE_F9       : mnemonic = "F9";        break;
        case KEY_CODE_F10      : mnemonic = "F10";       break;
        case KEY_CODE_F11      : mnemonic = "F11";       break;
        case KEY_CODE_F12      : mnemonic = "F12";       break;
        default: break;
    }
    if (state[0] == 0) { state[0] = 0x20; } // (int)strlen(state) - 1 below
    droid_key_text_string_t r = {};
    if (mnemonic == null) {
        snprintf0(r.text, "flags=0x%08X %.*s ch=%d 0x%08X %c", flags, (int)strlen(state) - 1, state, ch, ch, ch);
    } else {
        snprintf0(r.text, "flags=0x%08X %.*s ch=%d 0x%08X %s", flags, (int)strlen(state) - 1, state, ch, ch, mnemonic);
    }
    return r;
}

int droid_keys_translate(int32_t key_code, int32_t meta_state, int32_t flags) {
    int kc = 0;
    int numpad  = 1;
    int numlock  = (meta_state & AMETA_NUM_LOCK_ON) != 0;
    int capslock = (meta_state & AMETA_CAPS_LOCK_ON) != 0;
    switch (key_code) {
        case AKEYCODE_SHIFT_LEFT     : kc = KEY_CODE_SHIFT; break;
        case AKEYCODE_CTRL_LEFT      : kc = KEY_CODE_CTRL;  break;
        case AKEYCODE_ALT_LEFT       : kc = KEY_CODE_ALT;   break;
        case AKEYCODE_SOFT_LEFT:  case AKEYCODE_META_LEFT:
        case AKEYCODE_DPAD_LEFT      : kc = KEY_CODE_LEFT; break;
        case AKEYCODE_SOFT_RIGHT: case AKEYCODE_SHIFT_RIGHT:
        case AKEYCODE_ALT_RIGHT:  case AKEYCODE_META_RIGHT: case AKEYCODE_CTRL_RIGHT:
        case AKEYCODE_DPAD_RIGHT     : kc = KEY_CODE_RIGHT;     break;
        case AKEYCODE_DPAD_UP        : kc = KEY_CODE_UP;        break;
        case AKEYCODE_DPAD_DOWN      : kc = KEY_CODE_DOWN;      break;
        case AKEYCODE_PAGE_UP        : kc = KEY_CODE_PAGE_UP;   break;
        case AKEYCODE_PAGE_DOWN      : kc = KEY_CODE_PAGE_DOWN; break;
        case AKEYCODE_MOVE_HOME      : kc = KEY_CODE_HOME;      break;
        case AKEYCODE_MOVE_END       : kc = KEY_CODE_END;       break;
        case AKEYCODE_ENTER          : kc = KEY_CODE_ENTER;     break;
        case AKEYCODE_BACK           : kc = KEY_CODE_BACK;      break;
        case AKEYCODE_ESCAPE         : kc = KEY_CODE_ESC;       break;
        case AKEYCODE_FORWARD_DEL    : kc = KEY_CODE_DEL;       break;
        case AKEYCODE_DEL            : kc = KEY_CODE_BACKSPACE; break;
        case AKEYCODE_NUMPAD_0       : kc = numlock ? '0' : KEY_CODE_INS;       break;
        case AKEYCODE_NUMPAD_1       : kc = numlock ? '1' : KEY_CODE_END;       break;
        case AKEYCODE_NUMPAD_2       : kc = numlock ? '2' : KEY_CODE_DOWN;      break;
        case AKEYCODE_NUMPAD_3       : kc = numlock ? '3' : KEY_CODE_PAGE_DOWN; break;
        case AKEYCODE_NUMPAD_4       : kc = numlock ? '4' : KEY_CODE_LEFT;      break;
        case AKEYCODE_NUMPAD_5       : kc = numlock ? '5' : KEY_CODE_CENTER;    break;
        case AKEYCODE_NUMPAD_6       : kc = numlock ? '6' : KEY_CODE_RIGHT;     break;
        case AKEYCODE_NUMPAD_7       : kc = numlock ? '7' : KEY_CODE_HOME;      break;
        case AKEYCODE_NUMPAD_8       : kc = numlock ? '8' : KEY_CODE_UP;        break;
        case AKEYCODE_NUMPAD_9       : kc = numlock ? '9' : KEY_CODE_PAGE_UP;   break;
        case AKEYCODE_NUMPAD_DOT     : kc = numlock ? '.' : KEY_CODE_DEL;       break;
        case AKEYCODE_NUMPAD_ENTER   : kc = KEY_CODE_ENTER; break;
        case AKEYCODE_NUMPAD_DIVIDE  : kc = '/'; break;
        case AKEYCODE_NUMPAD_MULTIPLY: kc = '*'; break;
        case AKEYCODE_NUMPAD_SUBTRACT: kc = '-'; break;
        case AKEYCODE_NUMPAD_ADD     : kc = '+'; break;
        case AKEYCODE_NUMPAD_COMMA   : kc = ','; break;
        case AKEYCODE_NUMPAD_EQUALS  : kc = '='; break;
        default: numpad = 0; break;
    }
    if (numpad) { flags |= KEYBOARD_NUMPAD; }
    if (meta_state & AMETA_SYM_ON)       { flags |= KEYBOARD_SYM; }
    if (meta_state & AMETA_SHIFT_ON)     { flags |= KEYBOARD_SHIFT; }
    if (meta_state & AMETA_ALT_ON)       { flags |= KEYBOARD_ALT; }
    if (meta_state & AMETA_CTRL_ON)      { flags |= KEYBOARD_CTRL; }
    if (meta_state & AMETA_FUNCTION_ON)  { flags |= KEYBOARD_FN; }
    if (meta_state & AMETA_NUM_LOCK_ON)  { flags |= KEYBOARD_NUMLOCK; }
    if (meta_state & AMETA_CAPS_LOCK_ON) { flags |= KEYBOARD_CAPSLOCK; }
    if (kc == 0) {
        if (AKEYCODE_A <= key_code && key_code <= AKEYCODE_Z) {
            int caps = ((flags & KEYBOARD_SHIFT) != 0) ^ capslock;
            kc = (key_code - AKEYCODE_A) + (caps ? 'A' : 'a');
        } else if (AKEYCODE_0 <= key_code && key_code <= AKEYCODE_9) {
            kc = (key_code - AKEYCODE_0) + '0';
        } else if (AKEYCODE_F1 <= key_code && key_code <= AKEYCODE_F12) {
            kc = (key_code - AKEYCODE_F1) * 0x01000000 + KEY_CODE_F1;
        } else {
            switch (key_code) {
                case AKEYCODE_SPACE:                kc = 0x20; break;
                case AKEYCODE_MINUS:                kc = '-';  break;
                case AKEYCODE_PLUS :                kc = '+';  break;
                case AKEYCODE_EQUALS:               kc = '=';  break;
                case AKEYCODE_COMMA:                kc = ',';  break;
                case AKEYCODE_PERIOD:               kc = '.';  break;
                case AKEYCODE_SEMICOLON:            kc = ';';  break;
                case AKEYCODE_APOSTROPHE:           kc = '\''; break;
                case AKEYCODE_GRAVE:                kc = '`';  break;
                case AKEYCODE_TAB:                  kc = '\t'; break;
                case AKEYCODE_LEFT_BRACKET:         kc = '[';  break;
                case AKEYCODE_RIGHT_BRACKET:        kc = ']';  break;
                case AKEYCODE_BACKSLASH:            kc = '\\'; break;
                case AKEYCODE_SLASH:                kc = '/';  break;
                case AKEYCODE_AT:                   kc = '@';  break;
                case AKEYCODE_POUND:                kc = '#';  break;
                case AKEYCODE_NUMPAD_LEFT_PAREN:    kc = '(';  break;
                case AKEYCODE_NUMPAD_RIGHT_PAREN:   kc = ')';  break;
                default: break;
            }
        }
    }
    if (flags & KEYBOARD_SHIFT) {
        if ((flags & KEYBOARD_NUMPAD) == 0) {
            switch (kc) { // US keyboard specific
                case '1': kc = '!'; break;
                case '3': kc = '#'; break;
                case '4': kc = '$'; break;
                case '5': kc = '%'; break;
                case '6': kc = '^'; break;
                case '7': kc = '&'; break;
                case '8': kc = '*'; break;
                case '9': kc = '('; break;
                case '0': kc = ')'; break;
                case ',': kc = '<'; break;
                case '.': kc = '>'; break;
                case '/': kc = '?'; break;
                case '=': kc = '+'; break;
                case '-': kc = '_'; break;
                default: break;
            }
        }
        switch (kc) { // US keyboard specific
            case '`': kc = '~'; break;
            case '[': kc = '{'; break;
            case ']': kc = '}'; break;
            case ';': kc = ':'; break;
            case '\\': kc = '|'; break;
            case '\'': kc = '"'; break;
            default: break;
        }
    }
    return kc;
}

end_c