#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>

/* vibe0 という名前の定義が DeviceTree にある場合のみ有効化 */
#if DT_NODE_EXISTS(DT_ALIAS(vibe0))

static const struct gpio_dt_spec vibe_motor = GPIO_DT_SPEC_GET(DT_ALIAS(vibe0), gpios);

#define VIBE_DURATION_MS 50 

/* 振動処理を非同期で実行するための設定 */
static void vibrate_work_handler(struct k_work *work) {
    gpio_pin_set_dt(&vibe_motor, 1);
    k_msleep(VIBE_DURATION_MS);
    gpio_pin_set_dt(&vibe_motor, 0);
}

K_WORK_DEFINE(vibe_work, vibrate_work_handler);

/* キー入力イベントの監視 */
static int vibe_event_listener(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    
    /* * ev->state: true(押し下げ) 
     * ev->keycode: 0x04('A'キー)
     */
    if (ev != NULL && ev->state && ev->keycode == 0x04) { 
        k_work_submit(&vibe_work);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(vibe_handler, vibe_event_listener);
ZMK_SUBSCRIPTION(vibe_handler, zmk_keycode_state_changed);

/* 初期化処理 */
static int vibe_init(void) {
    if (!device_is_ready(vibe_motor.port)) return -ENODEV;
    return gpio_pin_configure_dt(&vibe_motor, GPIO_OUTPUT_INACTIVE);
}

/* ZMK起動時にこの機能を登録 */
SYS_INIT(vibe_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif /* DT_NODE_EXISTS(DT_ALIAS(vibe0)) */
