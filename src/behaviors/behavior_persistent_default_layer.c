/*
 * Copyright (c) 2024 ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behaviour_persistent_default_layer

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

#include <string.h>
#include <zmk/behavior.h>
#include <zmk/keymap.h>
#include <zmk/events/activity_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zephyr/settings/settings.h>


#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_PDF)
struct behavior_persistent_default_layer_config {
    uint8_t default_layer;
};

struct behavior_persistent_default_layer_data {
    uint8_t active_layer;
};

/* ====== Properities ====== */
static uint8_t saved_layer = 0;
/* ====== Properities ====== */

/* ====== Settings ====== */
static int pdf_settings_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg) {
    const char *next;
    int rc;

    if (settings_name_steq(name, "layer", &next) && !next) {
        if (len != sizeof(saved_layer)) {
            return -EINVAL;
        }

        rc = read_cb(cb_arg, &saved_layer, sizeof(saved_layer));
        if (rc >= 0) {
            return 0;
        }

        return rc;
    }

    return -ENOENT;
}

static struct settings_handler pdf_settings_handler = {
    .name = "pdf",
    .h_set = pdf_settings_set,
};

static int pdf_settings_save(void) {
    int ret = settings_save_one("pdf/active", &saved_layer, sizeof(saved_layer));
    if (ret < 0) {
        LOG_WRN("Failed to save persistent default layer: %d", ret);
        return ret;
    }

    return 0;
}
/* ====== Settings ====== */

/* ====== Key Binding Handlers ====== */
static int pdf_binding_pressed(struct zmk_behavior_binding *binding, struct zmk_behavior_binding_event event) {
    saved_layer = binding->param1;

    // Switch to the specified layer
    zmk_keymap_layer_toggle(saved_layer);
    
    // Save the layer to persistent storage
    pdf_settings_save();

    return ZMK_BEHAVIOR_OPAQUE;
}

static int pdf_binding_released(struct zmk_behavior_binding *binding, struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_pdf_driver_api = {
    .binding_pressed = pdf_binding_pressed,
    .binding_released = pdf_binding_released,
    .locality = BEHAVIOR_LOCALITY_GLOBAL,
};
/* ====== Key Binding Handlers ====== */

/* ====== Event Listeners ====== */
static int pdf_activity_listener(const zmk_event_t *eh) {
    struct zmk_activity_state_changed *activity_ev = as_zmk_activity_state_changed(eh);
    
    // Restore saved layer when keyboard becomes active (wakes from sleep)
    if (activity_ev->state == ZMK_ACTIVITY_ACTIVE && saved_layer > 0) {
        zmk_keymap_layer_toggle(saved_layer);
    }
    
    return 0;
}

ZMK_LISTENER(pdf_activity, pdf_activity_listener);
ZMK_SUBSCRIPTION(pdf_activity, zmk_activity_state_changed);
/* ====== Event Listeners ====== */

/* ====== Initialization ====== */
static int pdf_init(const struct device *dev) {
    LOG_DBG("Initializing persistent default layer (pdf) behavior");
    
    // Register settings handler
    settings_register(&pdf_settings_handler);
    
    // Load settings from persistent storage
    settings_load_subtree("pdf");
    
    // If a layer was saved, activate it
    if (saved_layer > 0) {
        zmk_keymap_layer_toggle(saved_layer);
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
/* ====== Initialization ====== */
#endif