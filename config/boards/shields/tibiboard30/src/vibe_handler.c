#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>

#if DT_NODE_EXISTS(DT_ALIAS(vibe0))

static const struct gpio_dt_spec vibe_motor = GPIO_DT_SPEC_GET(DT_ALIAS(vibe0), gpios);

/* 振動を処理するワークキュー */
static void vibrate_work_handler(struct k_work *work) {
    gpio_pin_set_dt(&vibe_motor, 1);
    k_msleep(100); /* 100msに少し長めに設定 */
    gpio_pin_set_dt(&vibe_motor, 0);
}

K_WORK_DEFINE(vibe_work, vibrate_work_handler);

/* 全てのキー入力を監視するリスナー */
static int vibe_event_listener(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    
    /* stateがtrue（キーが押された）なら何でも震えさせる */
    if (ev != NULL && ev->state) { 
        k_work_submit(&vibe_work);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(vibe_handler, vibe_event_listener);
ZMK_SUBSCRIPTION(vibe_handler, zmk_keycode_state_changed);

/* 初期化時にテストで1回震わせる */
static int vibe_init(void) {
    if (!device_is_ready(vibe_motor.port)) return -ENODEV;
    gpio_pin_configure_dt(&vibe_motor, GPIO_OUTPUT_INACTIVE);

    /* 起動テスト: 2回短く震わせる */
    gpio_pin_set_dt(&vibe_motor, 1);
    k_busy_wait(100000); /* 100ms */
    gpio_pin_set_dt(&vibe_motor, 0);
    
    return 0;
}

SYS_INIT(vibe_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif
