#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h> /* ここを変更：キーコードではなくレイヤー監視 */
#include <zmk/battery.h>

/* XIAO BLEのLED定義 */
static const struct gpio_dt_spec led_red   = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec led_green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec led_blue  = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

/* バッテリー確認用レイヤー番号 (7番を使用) */
#define BATTERY_LAYER_INDEX 7

#define LED_SHOW_TIME 2000
static struct k_work_delayable led_off_work;

static void all_leds_off(void) {
    gpio_pin_set_dt(&led_red, 1);
    gpio_pin_set_dt(&led_green, 1);
    gpio_pin_set_dt(&led_blue, 1);
}

static void led_off_handler(struct k_work *work) {
    all_leds_off();
}

static void show_battery_level(void) {
    uint8_t level = zmk_battery_state_of_charge();
    
    k_work_cancel_delayable(&led_off_work);
    all_leds_off();

    if (level >= 80) {
        gpio_pin_set_dt(&led_green, 0); /* 緑 */
    } else if (level >= 30) {
        gpio_pin_set_dt(&led_blue, 0);  /* 青 */
    } else {
        gpio_pin_set_dt(&led_red, 0);   /* 赤 */
    }

    k_work_schedule(&led_off_work, K_MSEC(LED_SHOW_TIME));
}

/* レイヤー変更イベントのリスナー */
static int battery_led_listener(const zmk_event_t *eh) {
    const struct zmk_layer_state_changed *ev = as_zmk_layer_state_changed(eh);
    
    if (ev != NULL) {
        /* 「バッテリー確認用レイヤー(7番)」が有効になった(state=true)瞬間だけ実行 */
        if (ev->layer == BATTERY_LAYER_INDEX && ev->state == true) {
            show_battery_level();
        }
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(battery_led, battery_led_listener);
ZMK_SUBSCRIPTION(battery_led, zmk_layer_state_changed);

static int battery_led_init(void) {
    if (!device_is_ready(led_red.port)) return -ENODEV;
    if (!device_is_ready(led_green.port)) return -ENODEV;
    if (!device_is_ready(led_blue.port)) return -ENODEV;

    gpio_pin_configure_dt(&led_red, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led_green, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led_blue, GPIO_OUTPUT_INACTIVE);

    all_leds_off();
    k_work_init_delayable(&led_off_work, led_off_handler);
    return 0;
}

SYS_INIT(battery_led_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
