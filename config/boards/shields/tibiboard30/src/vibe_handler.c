#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>

/* vibe0 がある場合のみ有効化 */
#if DT_NODE_EXISTS(DT_ALIAS(vibe0))

static const struct gpio_dt_spec vibe_motor = GPIO_DT_SPEC_GET(DT_ALIAS(vibe0), gpios);

/* * 振動の強さ・長さの調整 
 * ここを大きくすると、より長く（結果的に強く）感じます。
 * 150〜300 くらいがおすすめです。
 */
#define VIBE_DURATION_MS 200 

/* 振動処理 */
static void vibrate_work_handler(struct k_work *work) {
    gpio_pin_set_dt(&vibe_motor, 1);
    k_msleep(VIBE_DURATION_MS);
    gpio_pin_set_dt(&vibe_motor, 0);
}

K_WORK_DEFINE(vibe_work, vibrate_work_handler);

/* キー入力監視 */
static int vibe_event_listener(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    
    /* * 'A' キー (keycode 0x04) が押された(state=true)時だけ振動
     */
    if (ev != NULL && ev->state && ev->keycode == 0x04) { 
        k_work_submit(&vibe_work);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(vibe_handler, vibe_event_listener);
ZMK_SUBSCRIPTION(vibe_handler, zmk_keycode_state_changed);

/* 初期化 & 起動時テスト */
static int vibe_init(void) {
    if (!device_is_ready(vibe_motor.port)) return -ENODEV;
    gpio_pin_configure_dt(&vibe_motor, GPIO_OUTPUT_INACTIVE);

    /* 起動確認用：長めに0.5秒震わせる */
    gpio_pin_set_dt(&vibe_motor, 1);
    k_busy_wait(500000); /* 500ms */
    gpio_pin_set_dt(&vibe_motor, 0);
    
    return 0;
}

SYS_INIT(vibe_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif
