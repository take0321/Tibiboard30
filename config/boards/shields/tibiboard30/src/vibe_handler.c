#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>

#include <zmk/event_manager.h>
#include <zmk/ble.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/layer_state_changed.h>

#if DT_NODE_EXISTS(DT_ALIAS(vibe0))

static const struct gpio_dt_spec vibe_motor = GPIO_DT_SPEC_GET(DT_ALIAS(vibe0), gpios);

/* ==========================================
 * 設定エリア (ミリ秒)
 * ========================================== */

/* 機能1: レイヤー変更 */
#define LAYER_VIBE_MS    150   /* 少し長めに設定 (前回80ms -> 今回150ms) */

/* 機能2: プロファイル変更 */
#define PROF_START_DELAY 500   /* 変更後の待ち時間 */
#define PROF_START_MS    100   /* 開始合図 (0.1秒) */
#define PROF_SUCCESS_MS  1000  /* 接続成功 (1秒) */
#define PROF_POLL_MS     500   /* 接続チェック間隔 */


/* ワークキュー定義 */
static struct k_work_delayable layer_work;
static struct k_work_delayable prof_start_work;
static struct k_work_delayable prof_poll_work;
static struct k_work_delayable prof_success_work;


/* ------------------------------------------------
 * 機能1: レイヤー変更時の処理
 * ------------------------------------------------ */
static void layer_vibe_handler(struct k_work *work) {
    gpio_pin_set_dt(&vibe_motor, 1);
    k_msleep(LAYER_VIBE_MS);
    gpio_pin_set_dt(&vibe_motor, 0);
}

static int layer_listener(const zmk_event_t *eh) {
    const struct zmk_layer_state_changed *ev = as_zmk_layer_state_changed(eh);
    
    /* レイヤーが有効化(state=true)されたら振動 */
    if (ev != NULL && ev->state == true) {
        k_work_schedule(&layer_work, K_NO_WAIT);
    }
    return ZMK_EV_EVENT_BUBBLE;
}


/* ------------------------------------------------
 * 機能2: プロファイル変更時の処理
 * ------------------------------------------------ */

/* 2-3. 接続成功時 (1秒) */
static void prof_success_handler(struct k_work *work) {
    gpio_pin_set_dt(&vibe_motor, 1);
    k_msleep(PROF_SUCCESS_MS);
    gpio_pin_set_dt(&vibe_motor, 0);
}

/* 2-2. 接続監視ループ */
static void prof_poll_handler(struct k_work *work) {
    if (zmk_ble_active_profile_is_connected()) {
        k_work_schedule(&prof_success_work, K_NO_WAIT);
        return;
    }
    k_work_schedule(&prof_poll_work, K_MSEC(PROF_POLL_MS));
}

/* 2-1. 開始合図 (0.1秒 × 1回) */
static void prof_start_handler(struct k_work *work) {
    gpio_pin_set_dt(&vibe_motor, 1);
    k_msleep(PROF_START_MS);
    gpio_pin_set_dt(&vibe_motor, 0);

    /* 合図が終わったら接続監視へ */
    k_work_schedule(&prof_poll_work, K_NO_WAIT);
}

/* プロファイル変更イベント */
static int profile_listener(const zmk_event_t *eh) {
    const struct zmk_ble_active_profile_changed *ev = as_zmk_ble_active_profile_changed(eh);
    
    if (ev != NULL) {
        /* 他の動作をキャンセル */
        k_work_cancel_delayable(&prof_start_work);
        k_work_cancel_delayable(&prof_poll_work);
        k_work_cancel_delayable(&prof_success_work);
        gpio_pin_set_dt(&vibe_motor, 0);

        /* 0.5秒後に開始 */
        k_work_schedule(&prof_start_work, K_MSEC(PROF_START_DELAY));
    }
    return ZMK_EV_EVENT_BUBBLE;
}


/* ==========================================
 * 初期化と登録
 * ========================================== */

ZMK_LISTENER(vibe_layer, layer_listener);
ZMK_SUBSCRIPTION(vibe_layer, zmk_layer_state_changed);

ZMK_LISTENER(vibe_profile, profile_listener);
ZMK_SUBSCRIPTION(vibe_profile, zmk_ble_active_profile_changed);

static int vibe_init(void) {
    if (!device_is_ready(vibe_motor.port)) return -ENODEV;
    gpio_pin_configure_dt(&vibe_motor, GPIO_OUTPUT_INACTIVE);

    k_work_init_delayable(&layer_work, layer_vibe_handler);
    k_work_init_delayable(&prof_start_work, prof_start_handler);
    k_work_init_delayable(&prof_poll_work, prof_poll_handler);
    k_work_init_delayable(&prof_success_work, prof_success_handler);

    return 0;
}

SYS_INIT(vibe_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif
