#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/kernel.h>

#include <zmk/events/activity_state_changed.h>
#include <zmk/activity.h>
#include <zmk/keymap.h>

#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/hid_indicators_changed.h>
#include <zmk/events/split_peripheral_status_changed.h>
#include <zmk/events/layer_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_PDF)
    /* ====== Properties ====== */
    static zmk_keymap_layer_id_t persistent_layer = 0; // Standardize to default layer index 0
    static bool has_stored_layer = false;

    // Unified work structure to handle deferred/debounced layer enforcement
    static struct k_work_delayable pdf_restore_work;
    
    // The debounce window delay (in milliseconds)
    #define PDF_DEBOUNCE_DELAY_MS 50
    /* ====== Properties ====== */


    zmk_keymap_layer_id_t pdf_get_persistent_layer(void) {
        return persistent_layer;
    }

    /* ====== Settings ====== */
    #define SETTINGS_PARTITION "pdf"
    #define SETTINGS_KEY_PERSISTENT_LAYER "persistent_layer"
    #define SETTINGS_KEY "pdf/persistent_layer"

    // This single callback handler runs on the system workqueue when the debounce window expires
    static void pdf_restore_work_handler(struct k_work *work) {
        if (!has_stored_layer) {
            return;
        }

        zmk_keymap_layer_id_t target = pdf_get_persistent_layer();

        // CORRECTED: Check if our target layer is NOT currently the active layer.
        // If it isn't active, forcefully snap the keymap back to it.
        if (!zmk_keymap_layer_active(target)) {
            LOG_DBG("Debounce complete. Enforcing persistent layer index: %d", target);
            zmk_keymap_layer_to(target);
        }
    }

    static int pdf_settings_load(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg) {
        const char *next;
        int rc;

        if (settings_name_steq(name, SETTINGS_KEY_PERSISTENT_LAYER, &next) && !next) {
            rc = read_cb(cb_arg, &persistent_layer, sizeof(persistent_layer));
            if (rc >= 0) {
                has_stored_layer = true;
                // Initial boot load needs a slightly longer window to let structural layers stabilize
                k_work_reschedule(&pdf_restore_work, K_MSEC(150));
                return 0;
            }
            return rc;
        }

        return -ENOENT;
    }

    static struct settings_handler pdf_settings_handler = {
        .name = SETTINGS_PARTITION,
        .h_set = pdf_settings_load,
    };

    int pdf_switch_to_persistent_layer(void) {
        if (has_stored_layer) {
            k_work_reschedule(&pdf_restore_work, K_NO_WAIT);
        }
        return 0;
    }

    int pdf_settings_save(void) {
        int ret = settings_save_one(SETTINGS_KEY, &persistent_layer, sizeof(persistent_layer));
        if (ret < 0) {
            LOG_WRN("Failed to save persistent default layer: %d", ret);
            return ret;
        }
        return 0;
    }

    int pdf_switch_and_save(zmk_keymap_layer_id_t layer) {
        persistent_layer = layer;
        has_stored_layer = true;
        pdf_switch_to_persistent_layer();
        return pdf_settings_save();
    }
    /* ====== Settings ====== */

    /* ====== Event Listeners ====== */
    
    /* Activity State Listener */
    static int pdf_activity_listener(const zmk_event_t *eh) {
        struct zmk_activity_state_changed *activity_ev = as_zmk_activity_state_changed(eh);
        if (activity_ev->state == ZMK_ACTIVITY_ACTIVE) {
            LOG_DBG("Activity woke up; scheduling layer validation.");
            k_work_reschedule(&pdf_restore_work, K_MSEC(PDF_DEBOUNCE_DELAY_MS));
        }
        return 0;
    }
    ZMK_LISTENER(pdf_activity, pdf_activity_listener);
    ZMK_SUBSCRIPTION(pdf_activity, zmk_activity_state_changed);

    /* BLE Profile Change Listener */
    static int pdf_ble_profile_listener(const zmk_event_t *eh) {
        LOG_DBG("BLE profile shifted; scheduling layer validation.");
        k_work_reschedule(&pdf_restore_work, K_MSEC(PDF_DEBOUNCE_DELAY_MS));
        return 0;
    }
    ZMK_LISTENER(pdf_ble_profile, pdf_ble_profile_listener);
    ZMK_SUBSCRIPTION(pdf_ble_profile, zmk_ble_active_profile_changed);

    /* Endpoint/Transport Change Listener */
    static int pdf_endpoint_listener(const zmk_event_t *eh) {
        LOG_DBG("Endpoint transport altered; scheduling layer validation.");
        k_work_reschedule(&pdf_restore_work, K_MSEC(PDF_DEBOUNCE_DELAY_MS));
        return 0;
    }
    ZMK_LISTENER(pdf_endpoint, pdf_endpoint_listener);
    ZMK_SUBSCRIPTION(pdf_endpoint, zmk_endpoint_changed);

    /* Split Peripheral Status Change Listener */
    static int pdf_split_peripheral_listener(const zmk_event_t *eh) {
        struct zmk_split_peripheral_status_changed *split_ev = as_zmk_split_peripheral_status_changed(eh);
        if (split_ev->connected) {
            LOG_DBG("Split side established connection; scheduling layer validation.");
            k_work_reschedule(&pdf_restore_work, K_MSEC(PDF_DEBOUNCE_DELAY_MS));
        }
        return 0;
    }
    ZMK_LISTENER(pdf_split_peripheral, pdf_split_peripheral_listener);
    ZMK_SUBSCRIPTION(pdf_split_peripheral, zmk_split_peripheral_status_changed);

    /* Direct Layer State Change Guard */
    static int pdf_layer_state_listener(const zmk_event_t *eh) {
        // CORRECTED: Safely evaluate if our persistent target layer has dropped out of active status
        if (has_stored_layer && !zmk_keymap_layer_active(pdf_get_persistent_layer())) {
            k_work_reschedule(&pdf_restore_work, K_MSEC(PDF_DEBOUNCE_DELAY_MS));
        }
        return 0;
    }
    ZMK_LISTENER(pdf_layer_state, pdf_layer_state_listener);
    ZMK_SUBSCRIPTION(pdf_layer_state, zmk_layer_state_changed);

    /* HID Indicators Listener */
    #if IS_ENABLED(CONFIG_ZMK_HID_INDICATORS)
        static int pdf_hid_indicators_listener(const zmk_event_t *eh) {
            LOG_DBG("HID connection indicators changed; scheduling layer validation.");
            k_work_reschedule(&pdf_restore_work, K_MSEC(PDF_DEBOUNCE_DELAY_MS));
            return 0;
        }
        ZMK_LISTENER(pdf_hid_indicators, pdf_hid_indicators_listener);
        ZMK_SUBSCRIPTION(pdf_hid_indicators, zmk_hid_indicators_changed);
    #endif
    /* ====== Event Listeners ====== */

    static int pdf_init(void) {
        LOG_DBG("Initializing decoupled persistent default layer module");
        
        // Initialize our unified delayable work structure
        k_work_init_delayable(&pdf_restore_work, pdf_restore_work_handler);

        // Register settings handler
        settings_register(&pdf_settings_handler);
        
        // Load settings from persistent storage
        settings_load_subtree(SETTINGS_PARTITION);

        return 0;
    }

    SYS_INIT(pdf_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
#endif