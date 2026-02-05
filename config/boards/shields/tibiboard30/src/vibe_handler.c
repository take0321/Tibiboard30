#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h> /* レイヤー変更イベント用 */

#if DT_NODE_EXISTS(DT_ALIAS(vibe0))

static const struct gpio_dt_spec vibe_motor = GPIO_DT_SPEC_GET(DT_ALIAS(vibe0), gpios);

/* 短くキレのある振動にする */
#define VIBE_DURATION_MS 80 

static void vibrate_work_handler(struct k_work *work) {
    gpio_pin_set_dt(&vibe_motor, 1);
    k_msleep(VIBE_DURATION_MS);
    gpio_pin_set_dt(&vibe_motor, 0);
}

K_WORK_DEFINE(vibe_work, vibrate_work_handler);

/* レイヤー変更を監視するリスナー */
static int vibe_event_listener(const zmk_event_t *eh) {
    /* レイヤー状態が変わったイベントかチェック */
    const struct zmk_layer_state_changed *ev = as_zmk_layer_state_changed(eh);
    
    if (ev != NULL) {
        /* * ev->state: trueなら「そのレイヤーに入った」、falseなら「抜けた」
         * ev->layer: レイヤー番号
         * * ここでは「レイヤー0以外に入った時」だけ震えるようにします
         */
        if (ev->state == true && ev->layer > 0) {
            k_work_submit(&vibe_work);
        }
    }
    return ZMK_EV_EVENT_BUBBLE;
}

/* 購読するイベントを「キーコード」から「レイヤー状態」に変更 */
ZMK_LISTENER(vibe_handler, vibe_event_listener);
ZMK_SUBSCRIPTION(vibe_handler, zmk_layer_state_changed);

static int vibe_init(void) {
    if (!device_is_ready(vibe_motor.port)) return -ENODEV;
    gpio_pin_configure_dt(&vibe_motor, GPIO_OUTPUT_INACTIVE);

    /* 起動時の合図 */
    gpio_pin_set_dt(&vibe_motor, 1);
    k_busy_wait(200000); /* 200ms */
    gpio_pin_set_dt(&vibe_motor, 0);
    
    return 0;
}

SYS_INIT(vibe_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif
