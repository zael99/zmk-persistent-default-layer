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
#include <zmk/persistent_layer.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zephyr/settings/settings.h>

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_PDF)
struct behavior_persistent_default_layer_config {
    uint8_t default_layer;
};

/* ====== Key Binding Handlers ====== */
static int pdf_binding_pressed(struct zmk_behavior_binding *binding, struct zmk_behavior_binding_event event) {
    // Switch to the specified layer
    zmk_keymap_layer_to(binding->param1);
    
    // Save the layer to persistent storage
    pdf_settings_save(binding->param1);

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

/* ====== Initialization ====== */
static int pdf_init(const struct device *dev) {
    return 0;
}

static struct behavior_persistent_default_layer_config behavior_pdf_config = {      
    .default_layer = DT_INST_PROP(n, default_layer),
};                                                                                      
                                                                                        
BEHAVIOR_DT_INST_DEFINE(0, pdf_init, NULL, NULL,
                        &behavior_pdf_config, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                        &behavior_pdf_driver_api);
/* ====== Initialization ====== */
#endif
