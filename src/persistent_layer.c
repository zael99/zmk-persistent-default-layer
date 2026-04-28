
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#include <zmk/events/activity_state_changed.h>
#include <zmk/activity.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zephyr/settings/settings.h>

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_PDF)
/* ====== Properities ====== */
static zmk_keymap_layer_id_t persistent_layer = 0;
/* ====== Properities ====== */

/* ====== Settings ====== */
#define SETTINGS_PARTITION "pdf"
#define SETTINGS_KEY_PERSISTENT_LAYER "persistent_layer"

static int pdf_load_settings(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg) {
    const char *next;
    int rc;

    if (settings_name_steq(name, SETTINGS_KEY_PERSISTENT_LAYER, &next) && !next) {
        if (len != sizeof(persistent_layer)) {
            return -EINVAL;
        }

        rc = read_cb(cb_arg, &persistent_layer, sizeof(persistent_layer));
        if (rc >= 0) {
            return 0;
        }

        return rc;
    }

    return -ENOENT;
}

static struct settings_handler pdf_settings_handler = {
    .name = "pdf",
    .h_set = pdf_load_settings,
};

int pdf_settings_save(zmk_keymap_layer_id_t layer) {
    int ret = settings_save_one(SETTINGS_KEY_PERSISTENT_LAYER, &layer, sizeof(layer));
    //int ret = settings_save_one(SETTINGS_PARTITION "/" SETTINGS_KEY_PERSISTENT_LAYER, &persistent_layer, sizeof(persistent_layer));
    if (ret < 0) {
        LOG_WRN("Failed to save persistent default layer: %d", ret);
        return ret;
    }

    persistent_layer = layer;

    return 0;
}
/* ====== Settings ====== */

/* ====== Event Listeners ====== */
static int pdf_activity_listener(const zmk_event_t *eh) {
    struct zmk_activity_state_changed *activity_ev = as_zmk_activity_state_changed(eh);
    
    // Restore saved layer when keyboard becomes active (wakes from sleep)
    if (activity_ev->state == ZMK_ACTIVITY_ACTIVE && persistent_layer > 0) {
        zmk_keymap_layer_to(persistent_layer);
    }
    
    return 0;
}

ZMK_LISTENER(pdf_activity, pdf_activity_listener);
ZMK_SUBSCRIPTION(pdf_activity, zmk_activity_state_changed);
/* ====== Event Listeners ====== */

static int pdf_init(const struct device *dev) {
    LOG_DBG("Initializing persistent default layer (pdf) behavior");
    
    // Register settings handler
    settings_register(&pdf_settings_handler);
    
    // Load settings from persistent storage
    settings_load_subtree(SETTINGS_PARTITION);
    
    // If a layer was saved, activate it
    if (persistent_layer != 0) {
        zmk_keymap_layer_to(persistent_layer);
    }

    return 0;
}

SYS_INIT(pdf_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
#endif
