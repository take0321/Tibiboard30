/* config/src/vibe_handler.c */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zmk/event_manager.h>
#include <zephyr/sys/util.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zmk/ble.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* --- モーターの存在チェック --- */
#if DT_NODE_EXISTS(DT_ALIAS(vib0))

    #define VIB_NODE DT_ALIAS(vib0)
    static const struct gpio_dt_spec motor = GPIO_DT_SPEC_GET(VIB_NODE, gpios);

    /* レイヤー振動の ON/OFF フラグ (F19で切替) */
    static bool is_layer_vibe_enabled = true;

    /* モーター停止用 */
    static void vib_off_handler(struct k_work *work) {
        gpio_pin_set_dt(&motor, 0);
    }
    K_WORK_DELAYABLE_DEFINE(vib_off_work, vib_off_handler);

    /* 振動開始関数 */
    static void vib_start(int ms) {
        k_work_cancel_delayable(&vib_off_work);
        gpio_pin_set_dt(&motor, 1);
        k_work_schedule(&vib_off_work, K_MSEC(ms));
    }

    /* 接続完了イベント */
    struct k_work connect_work;
    static void connect_work_handler(struct k_work *work) {
        /* ★ここが本命の「つながったよ！」合図★ */
        vib_start(1500); 
    }

    static void on_connected(struct bt_conn *conn, uint8_t err) {
        if (err) return;
        k_work_submit(&connect_work);
    }

    BT_CONN_CB_DEFINE(conn_callbacks) = {
        .connected = on_connected,
    };

    /* --- イベントリスナー --- */
    #if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
        #include <zmk/events/layer_state_changed.h>
        #include <zmk/events/ble_active_profile_changed.h>
        #include <zmk/events/keycode_state_changed.h>

        static int vibration_listener(const zmk_event_t *eh) {
            
            /* 1. キーコード検知 (リモコン機能) */
            const struct zmk_keycode_state_changed *keycode_ev = as_zmk_keycode_state_changed(eh);
            if (keycode_ev && keycode_ev->state) { 
                if (keycode_ev->usage_page == 0x07) { 
                    switch (keycode_ev->keycode) {
                        case 0x6E:
                            is_layer_vibe_enabled = !is_layer_vibe_enabled;
                            vib_start(1500); 
                            break;
                        case 0x6F: vib_start(300); break;
                        case 0x70: vib_start(600); break;
                        case 0x71: vib_start(1000); break;
                    }
                }
                return ZMK_EV_EVENT_BUBBLE;
            }

            /* 2. レイヤー変更 */
            const struct zmk_layer_state_changed *layer_ev = as_zmk_layer_state_changed(eh);
            if (layer_ev) {
                if (is_layer_vibe_enabled) {
                    if (layer_ev->state) { 
                        vib_start(250);
                    }
                }
                return ZMK_EV_EVENT_BUBBLE;
            }

            /* 3. プロファイル変更 */
            /* ★ここを修正しました★ */
            const struct zmk_ble_active_profile_changed *profile_ev = as_zmk_ble_active_profile_changed(eh);
            if (profile_ev) {
                if (zmk_ble_active_profile_is_connected()) {
                    /* すでに接続されている場合、on_connected 側で振動するので */
                    /* ここでは振動させないことで重複を防ぎます */
                } else {
                    /* 切断された（他のプロファイルに移った）時だけ短く振動 */
                    vib_start(250); 
                }
                return ZMK_EV_EVENT_BUBBLE;
            }
            return ZMK_EV_EVENT_BUBBLE;
        }

        ZMK_LISTENER(vibration_listener, vibration_listener);
        ZMK_SUBSCRIPTION(vibration_listener, zmk_layer_state_changed);
        ZMK_SUBSCRIPTION(vibration_listener, zmk_ble_active_profile_changed);
        ZMK_SUBSCRIPTION(vibration_listener, zmk_keycode_state_changed);

    #endif

    /* 初期化 */
    static int vibration_init(const struct device *dev) {
        if (!gpio_is_ready_dt(&motor)) { return -ENODEV; }
        gpio_pin_configure_dt(&motor, GPIO_OUTPUT_INACTIVE);
        k_work_init(&connect_work, connect_work_handler);
        
        /* 起動時は短く1回だけ */
        vib_start(250); 
        return 0;
    }
    SYS_INIT(vibration_init, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);

#endif
