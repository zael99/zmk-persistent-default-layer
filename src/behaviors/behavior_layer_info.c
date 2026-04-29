/*
 * Copyright (c) 2024 ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behaviour_layer_info

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#include <string.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/behavior.h>
#include <zmk/keymap.h>
#include <zmk/hid.h>
#include <zmk/persistent_layer.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_LAYER_INFO)

#include <zmk/keys.h>

/* Helper function to convert ASCII character to ZMK keycode and modifiers */
typedef struct {
    uint32_t keycode;
    uint8_t mods;
} keycode_info_t;

static keycode_info_t ascii_to_keycode(uint8_t ascii_char) {
    keycode_info_t result = {0, 0};
    
    // Numbers 0-9
    if (ascii_char >= '0' && ascii_char <= '9') {
        if (ascii_char == '0') {
            result.keycode = HID_KEY_0;
        } else {
            result.keycode = HID_KEY_1 + (ascii_char - '1');
        }
        return result;
    }
    
    // Space
    if (ascii_char == ' ') {
        result.keycode = HID_KEY_SPACE;
        return result;
    }
    
    // Comma
    if (ascii_char == ',') {
        result.keycode = HID_KEY_COMMA;
        return result;
    }
    
    // Colon (Shift+;)
    if (ascii_char == ':') {
        result.keycode = HID_KEY_SEMICOLON;
        result.mods = MOD_LSFT;
        return result;
    }
    
    // Letters A-Z (lowercase)
    if (ascii_char >= 'a' && ascii_char <= 'z') {
        result.keycode = HID_KEY_A + (ascii_char - 'a');
        return result;
    }
    
    // Letters A-Z (uppercase with shift)
    if (ascii_char >= 'A' && ascii_char <= 'Z') {
        result.keycode = HID_KEY_A + (ascii_char - 'A');
        result.mods = MOD_LSFT;
        return result;
    }
    
    return result;  // Unsupported or default
}

/* Helper to send a keycode using zmk_behavior_invoke_binding */
static int send_keycode(keycode_info_t key_info) {
    zmk_hid_keyboard_press(key_info.keycode);
    k_sleep(K_MSEC(5));
    zmk_hid_keyboard_release(key_info.keycode);
    k_sleep(K_MSEC(5));
    /*struct zmk_behavior_binding binding = {
        .behavior_dev = "kp",  // key-press behavior
        .param1 = key_info.keycode,
        .param2 = key_info.mods,
    };
    
    struct zmk_behavior_binding_event event = {
        .position = 0,
        .timestamp = k_uptime_get(),
    };
    
    // Invoke the binding for key press
    zmk_behavior_invoke_binding(&binding, event, true);
    
    // Small delay
    k_sleep(K_MSEC(5));
    
    // Invoke the binding for key release
    zmk_behavior_invoke_binding(&binding, event, false);
    
    // Delay between key presses
    k_sleep(K_MSEC(5));*/
    
    return 0;
}

/* Main output function - formats and sends layer info using ZMK macros */
static int send_layer_info(void) {
    // Get current active layer
    zmk_keymap_layer_id_t active_layer = zmk_keymap_highest_layer_active();
    
    // Get saved persistent (default) layer
    zmk_keymap_layer_id_t persistent_layer = pdf_get_persistent_layer();
    
    // Build format string: "Active: X, Default: Y"
    char buffer[30];
    snprintf(buffer, sizeof(buffer), "Active: %d, Default: %d", active_layer, persistent_layer);
    
    LOG_INF("Layer info: %s", buffer);
    
    // Send each character using ZMK macros
    for (int i = 0; buffer[i] != '\0'; i++) {
        uint8_t ch = buffer[i];
        keycode_info_t key_info = ascii_to_keycode(ch);
        
        if (key_info.keycode == 0) {
            LOG_DBG("Skipping unsupported character: %c (0x%02x)", ch, ch);
            continue;
        }
        
        send_keycode(key_info);
    }
    
    return 0;
}

/* ====== Key Binding Handlers ====== */
static int layer_info_binding_pressed(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    send_layer_info();
    return ZMK_BEHAVIOR_OPAQUE;
}

static int layer_info_binding_released(struct zmk_behavior_binding *binding,
                                       struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_layer_info_driver_api = {
    .binding_pressed = layer_info_binding_pressed,
    .binding_released = layer_info_binding_released,
    .locality = BEHAVIOR_LOCALITY_GLOBAL,
};
/* ====== Key Binding Handlers ====== */

/* ====== Initialization ====== */
static int layer_info_init(const struct device *dev) {
    return 0;
}

BEHAVIOR_DT_INST_DEFINE(0, layer_info_init, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                        &behavior_layer_info_driver_api);
/* ====== Initialization ====== */

#endif
#endif
