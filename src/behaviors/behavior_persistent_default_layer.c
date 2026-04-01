/*
 * Copyright (c) 2024 ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behaviour_persistent_default_layer

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <zmk/behavior.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zephyr/settings/settings.h>

struct behavior_persistent_default_layer_config {
    uint8_t default_layer;
};

struct behavior_persistent_default_layer_data {
    uint8_t active_layer;
};

// Settings subsystem handler
static int pdf_settings_set(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg)
{
    if (strcmp(key, "active") == 0) {
        ssize_t result = read_cb(cb_arg, &saved_layer, sizeof(saved_layer));
        if (result > 0) {
            LOG_DBG("Loaded saved layer: %u", saved_layer);
            return 0;
        }
    }
    return -ENOENT;
}

static struct settings_handler pdf_settings_handler = {
    .name = "pdf",
    .h_set = pdf_settings_set,
};

static uint8_t saved_layer = 0;

static int pdf_settings_save(void) {
    int ret = settings_save_one("pdf/active", &saved_layer, sizeof(saved_layer));
    if (ret < 0) {
        LOG_WRN("Failed to save persistent default layer: %d", ret);
        return ret;
    }
    LOG_DBG("Saved layer: %u", saved_layer);
    return 0;
}

static int pdf_binding_pressed(struct zmk_behavior_binding *binding, struct zmk_behavior_binding_event event) {
    LOG_DBG("pdf binding_pressed: layer=%u", binding->param1);

    // Switch to the specified layer
    zmk_keymap_layer_to(binding->param1, false);
    
    // Save the layer to persistent storage
    saved_layer = binding->param1;
    pdf_settings_save();

    return ZMK_BEHAVIOR_OPAQUE;
}

static int pdf_binding_released(struct zmk_behavior_binding *binding, struct zmk_behavior_binding_event event) {
    LOG_DBG("pdf binding_released: layer=%u", binding->param1);
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_pdf_driver_api = {
    .binding_pressed = pdf_binding_pressed,
    .binding_released = pdf_binding_released,
};

static int pdf_init(const struct device *dev) {
    LOG_DBG("Initializing persistent default layer (pdf) behavior");
    
    // Register settings handler
    settings_register(&pdf_settings_handler);
    
    // Load settings from persistent storage
    settings_load_subtree("pdf");
    
    // If a layer was saved, activate it
    if (saved_layer > 0) {
        LOG_DBG("Restoring saved layer: %u", saved_layer);
        zmk_keymap_layer_to(saved_layer, false);
    }

    return 0;
}

#define BEHAVIOR_PDF_INST(n)                                                                \
    static struct behavior_persistent_default_layer_config behavior_pdf_config_##n = {      \
        .default_layer = DT_INST_PROP(n, default_layer),                                    \
    };                                                                                      \
                                                                                            \
    BEHAVIOR_DT_INST_DEFINE(n, pdf_init, NULL, NULL,                                        \
                            &behavior_pdf_config_##n, POST_KERNEL,                          \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                            \
                            &behavior_pdf_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BEHAVIOR_PDF_INST)
