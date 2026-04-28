
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#include <zmk/events/activity_state_changed.h>
#include <zmk/activity.h>
#include <zmk/keymap.h>

#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/hid_indicators_changed.h>

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

zmk_keymap_layer_id_t pdf_get_persistent_layer(void) {
    return persistent_layer;
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

/* BLE Profile Change Listener */
static int pdf_ble_profile_listener(const zmk_event_t *eh) {
    struct zmk_ble_active_profile_changed *ble_ev = as_zmk_ble_active_profile_changed(eh);
    
    // Restore saved layer when BLE profile changes
    if (persistent_layer > 0) {
        LOG_DBG("BLE profile changed to %d, restoring persistent layer %d", ble_ev->index, persistent_layer);
        zmk_keymap_layer_to(persistent_layer);
    }
    
    return 0;
}

ZMK_LISTENER(pdf_ble_profile, pdf_ble_profile_listener);
ZMK_SUBSCRIPTION(pdf_ble_profile, zmk_ble_active_profile_changed);

/* Endpoint/Transport Change Listener */
static int pdf_endpoint_listener(const zmk_event_t *eh) {
    struct zmk_endpoint_changed *ep_ev = as_zmk_endpoint_changed(eh);
    
    // Restore saved layer when endpoint (USB/BLE transport) changes
    if (persistent_layer > 0) {
        LOG_DBG("Endpoint changed, restoring persistent layer %d", persistent_layer);
        zmk_keymap_layer_to(persistent_layer);
    }
    
    return 0;
}

ZMK_LISTENER(pdf_endpoint, pdf_endpoint_listener);
ZMK_SUBSCRIPTION(pdf_endpoint, zmk_endpoint_changed);

/* HID Indicators/Connection State Listener */
static int pdf_hid_indicators_listener(const zmk_event_t *eh) {
    struct zmk_hid_indicators_changed *hid_ev = as_zmk_hid_indicators_changed(eh);
    
    // Restore saved layer when HID indicators change (connection state)
    if (persistent_layer > 0) {
        LOG_DBG("HID indicators changed, restoring persistent layer %d", persistent_layer);
        zmk_keymap_layer_to(persistent_layer);
    }
    
    return 0;
}

ZMK_LISTENER(pdf_hid_indicators, pdf_hid_indicators_listener);
ZMK_SUBSCRIPTION(pdf_hid_indicators, zmk_hid_indicators_changed);
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
