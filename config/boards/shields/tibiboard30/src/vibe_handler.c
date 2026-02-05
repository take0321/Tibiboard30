#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>

#define VIBE_NODE DT_ALIAS(vibe0)
static const struct gpio_dt_spec vibe_motor = GPIO_DT_SPEC_GET(VIBE_NODE, gpios);

/* 振動パターンの設定 */
#define VIBE_DURATION_MS 50  /* 振動する時間 */

static void vibrate_work_handler(struct k_work *work) {
    gpio_pin_set_dt(&vibe_motor, 1);
    k_msleep(VIBE_DURATION_MS);
    gpio_pin_set_dt(&vibe_motor, 0);
}

K_WORK_DEFINE(vibe_work, vibrate_work_handler);

/* イベントリスナー：キーが押された時に呼ばれる */
static int vibe_event_listener(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    
    /* * ここで「特定のキー」を判定します。
     * 例として「F24」が押された時、または「全てのキー」で振動させる設定が可能です。
     */
    if (ev != NULL && ev->state) { /* キーが押された(state=true)瞬間 */
        
        // 全てのキー入力で振動させたい場合は、以下の if 文を外してください
        if (ev->keycode == 0x73) { /* 0x73 = F24 キーコード */
            k_work_submit(&vibe_work);
        }
    }
    return ZMK_EV_EVENT_BUBBLE;
}

/* ZMKのイベントバスに登録 */
ZMK_LISTENER(vibe_handler, vibe_event_listener);
ZMK_SUBSCRIPTION(vibe_handler, zmk_keycode_state_changed);

/* 初期化 */
static int vibe_init(void) {
    if (!device_is_ready(vibe_motor.port)) return -ENODEV;
    gpio_pin_configure_dt(&vibe_motor, GPIO_OUTPUT_INACTIVE);
    return 0;
}

/* アプリケーション開始時に実行 */
SYS_INIT(vibe_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
