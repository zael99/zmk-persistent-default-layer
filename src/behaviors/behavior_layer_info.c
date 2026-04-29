/*
 * Copyright (c) 2024 ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behaviour_layer_info

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

#include <string.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/behavior.h>
#include <zmk/keymap.h>
#include <zmk/hid.h>
#include <zmk/persistent_layer.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_LAYER_INFO)

#include <zmk/keys.h>

/* Helper function to convert ASCII character to HID keycode and modifier */
typedef struct {
    uint8_t keycode;
    uint8_t modifier;
} hid_keycode_t;

static hid_keycode_t ascii_to_hid(uint8_t ascii_char) {
    hid_keycode_t result = {0, 0};
    
    // Numbers 0-9
    if (ascii_char >= '0' && ascii_char <= '9') {
        result.keycode = HID_KEY_1 + (ascii_char - '0' + 9) % 10;
        if (ascii_char == '0') {
            result.keycode = HID_KEY_0;
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
        result.modifier = 0x02;  // Left shift
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
        result.modifier = 0x02;  // Left shift
        return result;
    }
    
    return result;  // Unsupported or default
}

/* Helper to send a single keypress via HID */
static int send_hid_report(hid_keycode_t key_info) {
    struct zmk_hid_keyboard_report report = {
        .report_id = ZMK_HID_REPORT_ID_KEYBOARD,
        .body = {
            .modifiers = key_info.modifier,
            .reserved = 0,
            .keys = {key_info.keycode, 0, 0, 0, 0, 0}
        }
    };

    zmk_hid_keyboard_report(&report.body);
    zmk_hid_keyboard_report_clear();

    return 0;
}

/* Main output function - formats and sends layer info */
static int send_layer_info(void) {
    // Get current active layer
    zmk_keymap_layer_id_t active_layer = zmk_keymap_highest_layer_active();
    
    // Get saved persistent (default) layer
    zmk_keymap_layer_id_t persistent_layer = pdf_get_persistent_layer();
    
    // Build format string: "Active: X, Default: Y"
    char buffer[30];
    snprintf(buffer, sizeof(buffer), "Active: %d, Default: %d", active_layer, persistent_layer);
    
    LOG_DBG("Layer info output: %s", buffer);
    
    // Send each character
    for (int i = 0; buffer[i] != '\0'; i++) {
        uint8_t ch = buffer[i];
        hid_keycode_t key_info = ascii_to_hid(ch);
        
        if (key_info.keycode == 0) {
            LOG_DBG("Skipping unsupported character: %c (0x%02x)", ch, ch);
            continue;
        }
        
        send_hid_report(key_info);
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
