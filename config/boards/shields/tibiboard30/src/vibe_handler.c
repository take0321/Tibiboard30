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

/* --- 1. モーターの存在チェック --- */
/* alias "vib0" があるか確認。無い場合（右側など）は中身を空っぽにする */
#if DT_NODE_EXISTS(DT_ALIAS(vib0))

    #define VIB_NODE DT_ALIAS(vib0)
    static const struct gpio_dt_spec motor = GPIO_DT_SPEC_GET(VIB_NODE, gpios);

    /* --- ここからメイン処理 --- */

    static void vib_off_handler(struct k_work *work) {
        gpio_pin_set_dt(&motor, 0);
    }
    K_WORK_DELAYABLE_DEFINE(vib_off_work, vib_off_handler);

    static void vib_start(int ms) {
        k_work_cancel_delayable(&vib_off_work);
        gpio_pin_set_dt(&motor, 1);
        k_work_schedule(&vib_off_work, K_MSEC(ms));
    }

    /* 接続イベント用の「後回し」ワーク */
    struct k_work connect_work;
    static void connect_work_handler(struct k_work *work) {
        vib_start(1500); /* 接続完了 1.5秒 */
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

        static int vibration_listener(const zmk_event_t *eh) {
            
            /* レイヤー変更 -> 0.15秒 */
            const struct zmk_layer_state_changed *layer_ev = as_zmk_layer_state_changed(eh);
            if (layer_ev) {
                if (layer_ev->state) { 
                    vib_start(150);
                }
                return ZMK_EV_EVENT_BUBBLE;
            }

            /* プロファイル変更 -> 接続状態に応じて */
            const struct zmk_ble_active_profile_changed *profile_ev = as_zmk_ble_active_profile_changed(eh);
            if (profile_ev) {
                if (zmk_ble_active_profile_is_connected()) {
                    vib_start(1500); /* 接続済みなら1.5秒 */
                } else {
                    vib_start(150);  /* 未接続なら0.15秒 */
                }
                return ZMK_EV_EVENT_BUBBLE;
            }
            return ZMK_EV_EVENT_BUBBLE;
        }

        ZMK_LISTENER(vibration_listener, vibration_listener);
        ZMK_SUBSCRIPTION(vibration_listener, zmk_layer_state_changed);
        ZMK_SUBSCRIPTION(vibration_listener, zmk_ble_active_profile_changed);
    #endif

    /* --- 初期化 --- */
    static int vibration_init(const struct device *dev) {
        if (!gpio_is_ready_dt(&motor)) {
            return -ENODEV;
        }
        
        gpio_pin_configure_dt(&motor, GPIO_OUTPUT_INACTIVE);
        k_work_init(&connect_work, connect_work_handler);

        /* 起動確認: 0.15秒 */
        vib_start(150);

        return 0;
    }
    SYS_INIT(vibration_init, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);

#endif /* DT_NODE_EXISTS(DT_ALIAS(vib0)) */
