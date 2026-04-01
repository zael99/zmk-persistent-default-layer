/*
 * Copyright (c) 2024 ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behaviour_persistent_default_layer

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zmk/behavior.h>
#include <zmk/matrix.h>
#include <zmk/keymap.h>
#include <zmk/keys.h>
#include <zmk/virtual_key_position.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zephyr/settings/settings.h>

struct behavior_persistent_default_layer_config {
    uint8_t default_layer;
};

struct behavior_persistent_default_layer_data {
    uint8_t active_layer;
};

// Settings subsystem handle
static struct settings_handler pdf_settings_handler = {
    .name = "pdf",
};

static uint8_t saved_layer = 0;

// Load settings from persistent storage
static int pdf_settings_load(
    const char *key, size_t len, settings_read_cb read_cb, void *cb_arg)
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

// Settings handler structure
static struct settings_handler_static pdf_handler = {
    .name = "pdf",
    .h_set = NULL,
    .h_commit = NULL,
    .h_export = NULL,
    .h_get = pdf_settings_load,
};

static int pdf_settings_save(void)
{
    int ret = settings_save_one("pdf/active", &saved_layer, sizeof(saved_layer));
    if (ret < 0) {
        LOG_WRN("Failed to save persistent default layer: %d", ret);
        return ret;
    }
    LOG_DBG("Saved layer: %u", saved_layer);
    return 0;
}

static int pdf_on_keydown(struct zmk_behavior_binding_event event,
                          struct zmk_behavior_binding *binding)
{
    LOG_DBG("pdf on_keydown: layer=%u", binding->param1);

    // Switch to the specified layer
    zmk_layer_on(binding->param1);
    
    // Save the layer to persistent storage
    saved_layer = binding->param1;
    pdf_settings_save();

    return ZMK_BEHAVIOR_OPAQUE;
}

static int pdf_on_keyup(struct zmk_behavior_binding_event event,
                        struct zmk_behavior_binding *binding)
{
    LOG_DBG("pdf on_keyup: layer=%u", binding->param1);
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_pdf_driver_api = {
    .binding_convert_central_state = NULL,
    .on_keydown = pdf_on_keydown,
    .on_keyup = pdf_on_keyup,
    .on_hold = NULL,
};

static int pdf_init(const struct device *dev)
{
    LOG_DBG("Initializing persistent default layer (pdf) behavior");
    
    // Register settings handler
    settings_register(&pdf_handler);
    
    // Load settings from persistent storage
    settings_load_subtree("pdf");
    
    // If a layer was saved, activate it
    if (saved_layer > 0) {
        LOG_DBG("Restoring saved layer: %u", saved_layer);
        zmk_layer_on(saved_layer);
    }

    return 0;
}

#define BEHAVIOR_PDF_INST(n)                                                    \
    static struct behavior_persistent_default_layer_config behavior_pdf_config_##n = { \
        .default_layer = DT_INST_PROP(n, default_layer),                      \
    };                                                                         \
                                                                               \
    static struct behavior_persistent_default_layer_data behavior_pdf_data_##n = { \
        .active_layer = 0,                                                     \
    };                                                                         \
                                                                               \
    BEHAVIOR_DEFINE(pdf, DT_INST(0, zmk_behaviour_persistent_default_layer),  \
                    &behavior_pdf_driver_api,                                 \
                    &behavior_pdf_data_##n,                                   \
                    &behavior_pdf_config_##n, pdf_init,                       \
                    POST_KERNEL, 81);

DT_INST_FOREACH_STATUS_OKAY(BEHAVIOR_PDF_INST)
