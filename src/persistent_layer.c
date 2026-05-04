
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

#include <zmk/events/activity_state_changed.h>
#include <zmk/activity.h>
#include <zmk/keymap.h>

#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/hid_indicators_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_PDF)
    /* ====== Properities ====== */
    static zmk_keymap_layer_id_t persistent_layer = NULL;
    /* ====== Properities ====== */

    /* ====== Settings ====== */
    #define SETTINGS_PARTITION "pdf"
    #define SETTINGS_KEY_PERSISTENT_LAYER "persistent_layer"
    #define SETTINGS_KEY "pdf/persistent_layer"

    static int pdf_settings_load(void) {
        zmk_keymap_layer_id_t stored_layer = 0;
        int ret = settings_read(SETTINGS_KEY, &stored_layer, sizeof(stored_layer));
        
        if (ret == 0) {
            LOG_DBG("Loaded persistent layer %d from storage", stored_layer);
        } else if (ret == -ENOENT) {
            LOG_DBG("No stored persistent layer found, using default");
        } else {
            LOG_WRN("Failed to load persistent layer, using default: %d", ret);
        }
        
        // If a layer was saved, activate it
        if (stored_layer != 0) {
            persistent_layer = stored_layer;
        } else {
            persistent_layer = 0;
        }

        return ret;
    }

    /*static int pdf_load_settings(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg) {
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
        .name = SETTINGS_PARTITION,
        .h_set = pdf_load_settings,
    };*/

    int pdf_settings_save(zmk_keymap_layer_id_t layer) {
        persistent_layer = layer;

        int ret = settings_save_one(SETTINGS_KEY, &persistent_layer, sizeof(persistent_layer));
        if (ret < 0) {
            LOG_WRN("Failed to save persistent default layer: %d", ret);
            return ret;
        }

        return 0;
    }

    zmk_keymap_layer_id_t pdf_get_persistent_layer(void) {
        return persistent_layer;
    }
    /* ====== Settings ====== */

    /* ====== Event Listeners ====== */
    static int pdf_activity_listener(const zmk_event_t *eh) {
        if (persistent_layer == NULL) {
            return 0;
        }

        struct zmk_activity_state_changed *activity_ev = as_zmk_activity_state_changed(eh);
        
        // Restore saved layer when keyboard becomes active (wakes from sleep)
        if (activity_ev->state == ZMK_ACTIVITY_ACTIVE) {
            LOG_DBG("Activity state changed to active, restoring persistent layer %d", persistent_layer);
            zmk_keymap_layer_to(persistent_layer);
        }
        
        return 0;
    }

    ZMK_LISTENER(pdf_activity, pdf_activity_listener);
    ZMK_SUBSCRIPTION(pdf_activity, zmk_activity_state_changed);

    /* BLE Profile Change Listener */
    static int pdf_ble_profile_listener(const zmk_event_t *eh) {
        if (persistent_layer == NULL) {
            return 0;
        }

        struct zmk_ble_active_profile_changed *ble_ev = as_zmk_ble_active_profile_changed(eh);
        
        // Restore saved layer when BLE profile changes
        LOG_DBG("BLE profile changed to %d, restoring persistent layer %d", ble_ev->index, persistent_layer);
        zmk_keymap_layer_to(persistent_layer);
        
        return 0;
    }

    ZMK_LISTENER(pdf_ble_profile, pdf_ble_profile_listener);
    ZMK_SUBSCRIPTION(pdf_ble_profile, zmk_ble_active_profile_changed);
    /* BLE Profile Change Listener */

    /* Endpoint/Transport Change Listener */
    static int pdf_endpoint_listener(const zmk_event_t *eh) {
        if (persistent_layer == NULL) {
            return 0;
        }

        __unused struct zmk_endpoint_changed *ep_ev = as_zmk_endpoint_changed(eh);
        
        // Restore saved layer when endpoint (USB/BLE transport) changes
        LOG_DBG("Endpoint changed, restoring persistent layer %d", persistent_layer);
        zmk_keymap_layer_to(persistent_layer);
        
        return 0;
    }

    ZMK_LISTENER(pdf_endpoint, pdf_endpoint_listener);
    ZMK_SUBSCRIPTION(pdf_endpoint, zmk_endpoint_changed);
    /* Endpoint/Transport Change Listener */

    /* HID Indicators/Connection State Listener */
    #if IS_ENABLED(CONFIG_ZMK_HID_INDICATORS)
        static int pdf_hid_indicators_listener(const zmk_event_t *eh) {
            if (persistent_layer == NULL) {
                return 0;
            }

            struct zmk_hid_indicators_changed *hid_ev = as_zmk_hid_indicators_changed(eh);
            
            // Restore saved layer when HID indicators change (connection state)
            LOG_DBG("HID indicators changed, restoring persistent layer %d", persistent_layer);
            zmk_keymap_layer_to(persistent_layer);
            
            return 0;
        }

        ZMK_LISTENER(pdf_hid_indicators, pdf_hid_indicators_listener);
        ZMK_SUBSCRIPTION(pdf_hid_indicators, zmk_hid_indicators_changed);
    #endif
    /* HID Indicators/Connection State Listener */
    /* ====== Event Listeners ====== */

    static int pdf_init(void) {
        LOG_DBG("Initializing persistent default layer (pdf) behavior");
        
        int ret = pdf_settings_load();
        zmk_keymap_layer_to(persistent_layer);

        return 0;
    }

    SYS_INIT(pdf_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
#endif
