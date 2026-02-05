#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <dt-bindings/zmk/keys.h> /* キー定義を使用 */

#define VIBE_NODE DT_ALIAS(vibe0)
static const struct gpio_dt_spec vibe_motor = GPIO_DT_SPEC_GET(VIBE_NODE, gpios);

#define VIBE_DURATION_MS 50 

static void vibrate_work_handler(struct k_work *work) {
    gpio_pin_set_dt(&vibe_motor, 1);
    k_msleep(VIBE_DURATION_MS);
    gpio_pin_set_dt(&vibe_motor, 0);
}

K_WORK_DEFINE(vibe_work, vibrate_work_handler);

static int vibe_event_listener(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    
    if (ev != NULL && ev->state) { 
        /* * ev->keycode は HID Usage ID です。
         * 'A' キーの ID は 0x04 です。
         */
        if (ev->keycode == 0x04) { 
            k_work_submit(&vibe_work);
        }
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(vibe_handler, vibe_event_listener);
ZMK_SUBSCRIPTION(vibe_handler, zmk_keycode_state_changed);

static int vibe_init(void) {
    if (!device_is_ready(vibe_motor.port)) return -ENODEV;
    gpio_pin_configure_dt(&vibe_motor, GPIO_OUTPUT_INACTIVE);
    return 0;
}

SYS_INIT(vibe_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
