#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>

#include <zmk/event_manager.h>
#include <zmk/ble.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/keycode_state_changed.h>

#if DT_NODE_EXISTS(DT_ALIAS(vibe0))

static const struct gpio_dt_spec vibe_motor = GPIO_DT_SPEC_GET(DT_ALIAS(vibe0), gpios);

/* ==========================================
 * 設定エリア (ミリ秒)
 * ========================================== */

/* 機能1: レイヤー変更 */
#define LAYER_VIBE_MS    300   /* 振動時間 */

/* 機能2: プロファイル変更 */
#define PROF_START_DELAY 100   /* 変更後の待ち時間 */
#define PROF_START_MS    300   /* 開始合図 */
#define PROF_SUCCESS_MS  1000  /* 接続成功 */
#define PROF_POLL_MS     500   /* 接続チェック間隔 */

/* 機能3: ON/OFF切り替え通知 (NEW!) */
#define TOGGLE_FEEDBACK_MS 1000 /* 切り替えたら1秒振動 */


/* ==========================================
 * グローバル変数
 * ========================================== */
static bool layer_vibe_enabled = true; /* デフォルトはON */


/* ==========================================
 * ワークキュー定義
 * ========================================== */
static struct k_work_delayable layer_work;
static struct k_work_delayable prof_start_work;
static struct k_work_delayable prof_poll_work;
static struct k_work_delayable prof_success_work;
static struct k_work_delayable toggle_work; /* 切り替え通知用 (追加) */


/* ------------------------------------------------
 * 振動実行処理 (実体)
 * ------------------------------------------------ */

/* レイヤー振動 */
static void layer_vibe_handler(struct k_work *work) {
    gpio_pin_set_dt(&vibe_motor, 1);
    k_msleep(LAYER_VIBE_MS);
    gpio_pin_set_dt(&vibe_motor, 0);
}

/* 切り替え通知 (1秒) */
static void toggle_feedback_handler(struct k_work *work) {
    gpio_pin_set_dt(&vibe_motor, 1);
    k_msleep(TOGGLE_FEEDBACK_MS);
    gpio_pin_set_dt(&vibe_motor, 0);
}

/* プロファイル変更関連 */
static void prof_success_handler(struct k_work *work) {
    gpio_pin_set_dt(&vibe_motor, 1);
    k_msleep(PROF_SUCCESS_MS);
    gpio_pin_set_dt(&vibe_motor, 0);
}

static void prof_poll_handler(struct k_work *work) {
    if (zmk_ble_active_profile_is_connected()) {
        k_work_schedule(&prof_success_work, K_NO_WAIT);
        return;
    }
    k_work_schedule(&prof_poll_work, K_MSEC(PROF_POLL_MS));
}

static void prof_start_handler(struct k_work *work) {
    gpio_pin_set_dt(&vibe_motor, 1);
    k_msleep(PROF_START_MS);
    gpio_pin_set_dt(&vibe_motor, 0);
    k_work_schedule(&prof_poll_work, K_NO_WAIT);
}


/* ------------------------------------------------
 * イベントリスナー (監視役)
 * ------------------------------------------------ */

/* 1. レイヤー変更監視 */
static int layer_listener(const zmk_event_t *eh) {
    const struct zmk_layer_state_changed *ev = as_zmk_layer_state_changed(eh);
    
    /* 振動機能がONの時だけ実行 */
    if (ev != NULL && ev->state == true && layer_vibe_enabled) {
        k_work_schedule(&layer_work, K_NO_WAIT);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

/* 2. プロファイル変更監視 */
static int profile_listener(const zmk_event_t *eh) {
    const struct zmk_ble_active_profile_changed *ev = as_zmk_ble_active_profile_changed(eh);
    
    if (ev != NULL) {
        k_work_cancel_delayable(&prof_start_work);
        k_work_cancel_delayable(&prof_poll_work);
        k_work_cancel_delayable(&prof_success_work);
        k_work_cancel_delayable(&toggle_work); 
        gpio_pin_set_dt(&vibe_motor, 0);

        k_work_schedule(&prof_start_work, K_MSEC(PROF_START_DELAY));
    }
    return ZMK_EV_EVENT_BUBBLE;
}

/* 3. キー入力監視 (ON/OFF切り替え用) */
static int key_listener(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    
    /* F19キー (0x6E) が押されたら切り替え */
    if (ev != NULL && ev->state && ev->keycode == 0x6E) { 
        layer_vibe_enabled = !layer_vibe_enabled; 
        
        /* 切り替え合図の振動 (1秒) を開始 */
        /* 他の短い振動と被らないよう、念のため前のをキャンセル */
        k_work_cancel_delayable(&layer_work);
        k_work_schedule(&toggle_work, K_NO_WAIT);
    }
    return ZMK_EV_EVENT_BUBBLE;
}


/* ==========================================
 * 登録処理
 * ========================================== */

ZMK_LISTENER(vibe_layer, layer_listener);
ZMK_SUBSCRIPTION(vibe_layer, zmk_layer_state_changed);

ZMK_LISTENER(vibe_profile, profile_listener);
ZMK_SUBSCRIPTION(vibe_profile, zmk_ble_active_profile_changed);

ZMK_LISTENER(vibe_key, key_listener);
ZMK_SUBSCRIPTION(vibe_key, zmk_keycode_state_changed);

static int vibe_init(void) {
    if (!device_is_ready(vibe_motor.port)) return -ENODEV;
    gpio_pin_configure_dt(&vibe_motor, GPIO_OUTPUT_INACTIVE);

    k_work_init_delayable(&layer_work, layer_vibe_handler);
    k_work_init_delayable(&prof_start_work, prof_start_handler);
    k_work_init_delayable(&prof_poll_work, prof_poll_handler);
    k_work_init_delayable(&prof_success_work, prof_success_handler);
    k_work_init_delayable(&toggle_work, toggle_feedback_handler); /* 追加 */

    return 0;
}

SYS_INIT(vibe_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif
