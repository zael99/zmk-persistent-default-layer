/*
 * Copyright (c) 2024 ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_default_layer

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zmk/behavior.h>
#include <zmk/matrix.h>
#include <zmk/keymap.h>
#include <zmk/keys.h>
#include <zmk/virtual_key_position.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zephyr/settings/settings.h>

struct behavior_default_layer_config {
    uint8_t default_layer;
};

struct behavior_default_layer_data {
    uint8_t active_layer;
};

// Settings subsystem handle
static struct settings_handler default_layer_settings_handler = {
    .name = "default_layer",
};

static uint8_t saved_layer = 0;

// Load settings from persistent storage
static int default_layer_settings_load(
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
static struct settings_handler_static default_layer_handler = {
    .name = "default_layer",
    .h_set = NULL,
    .h_commit = NULL,
    .h_export = NULL,
    .h_get = default_layer_settings_load,
};

static int default_layer_settings_save(void)
{
    int ret = settings_save_one("default_layer/active", &saved_layer, sizeof(saved_layer));
    if (ret < 0) {
        LOG_WRN("Failed to save default layer: %d", ret);
        return ret;
    }
    LOG_DBG("Saved layer: %u", saved_layer);
    return 0;
}

static int default_layer_on_keydown(struct zmk_behavior_binding_event event,
                                    struct zmk_behavior_binding *binding)
{
    LOG_DBG("default_layer on_keydown: layer=%u", binding->param1);

    // Switch to the specified layer
    zmk_layer_on(binding->param1);
    
    // Save the layer to persistent storage
    saved_layer = binding->param1;
    default_layer_settings_save();

    return ZMK_BEHAVIOR_OPAQUE;
}

static int default_layer_on_keyup(struct zmk_behavior_binding_event event,
                                  struct zmk_behavior_binding *binding)
{
    LOG_DBG("default_layer on_keyup: layer=%u", binding->param1);
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_default_layer_driver_api = {
    .binding_convert_central_state = NULL,
    .on_keydown = default_layer_on_keydown,
    .on_keyup = default_layer_on_keyup,
    .on_hold = NULL,
};

static int default_layer_init(const struct device *dev)
{
    LOG_DBG("Initializing default_layer behavior");
    
    // Register settings handler
    settings_register(&default_layer_handler);
    
    // Load settings from persistent storage
    settings_load_subtree("default_layer");
    
    // If a layer was saved, activate it
    if (saved_layer > 0) {
        LOG_DBG("Restoring saved layer: %u", saved_layer);
        zmk_layer_on(saved_layer);
    }

    return 0;
}

#define BEHAVIOR_DEFAULT_LAYER_INST(n)                                         \
    static struct behavior_default_layer_config behavior_default_layer_config_##n = { \
        .default_layer = DT_INST_PROP(n, default_layer),                      \
    };                                                                         \
                                                                               \
    static struct behavior_default_layer_data behavior_default_layer_data_##n = { \
        .active_layer = 0,                                                     \
    };                                                                         \
                                                                               \
    BEHAVIOR_DEFINE(default_layer, DT_INST(0, zmk_behavior_default_layer),   \
                    &behavior_default_layer_driver_api,                       \
                    &behavior_default_layer_data_##n,                         \
                    &behavior_default_layer_config_##n, default_layer_init,   \
                    POST_KERNEL, 81);

DT_INST_FOREACH_STATUS_OKAY(BEHAVIOR_DEFAULT_LAYER_INST)
