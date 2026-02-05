#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/battery.h>

/* XIAO BLEのオンボードLEDの定義 (Active Low: 0で点灯, 1で消灯) */
/* 赤(Led0), 緑(Led1), 青(Led2) */
static const struct gpio_dt_spec led_red   = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec led_green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec led_blue  = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

/* 点灯時間 (ms) */
#define LED_SHOW_TIME 2000

/* ワークキュー */
static struct k_work_delayable led_off_work;

/* LEDをすべて消す関数 */
static void all_leds_off(void) {
    gpio_pin_set_dt(&led_red, 1);   /* 1 = OFF */
    gpio_pin_set_dt(&led_green, 1);
    gpio_pin_set_dt(&led_blue, 1);
}

/* タイマー経過後にLEDを消す処理 */
static void led_off_handler(struct k_work *work) {
    all_leds_off();
}

/* バッテリー表示のメイン処理 */
static void show_battery_level(void) {
    uint8_t level = zmk_battery_state_of_charge();
    
    /* 既存のタイマーをリセットして消灯 */
    k_work_cancel_delayable(&led_off_work);
    all_leds_off();

    /* 残量に応じた色を点灯 (0 = ON) */
    if (level >= 80) {
        /* 80%以上: 緑 */
        gpio_pin_set_dt(&led_green, 0);
    } else if (level >= 30) {
        /* 30%〜79%: 青 */
        gpio_pin_set_dt(&led_blue, 0);
    } else {
        /* 30%未満: 赤 */
        gpio_pin_set_dt(&led_red, 0);
    }

    /* 2秒後に消す予約 */
    k_work_schedule(&led_off_work, K_MSEC(LED_SHOW_TIME));
}

/* キーイベントの監視 */
static int battery_led_listener(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    
    /* F20キー (keycode: 0x6F) が押されたら実行 */
    if (ev != NULL && ev->state) {
        if (ev->keycode == 0x6F) { /* F20 */
            show_battery_level();
        }
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(battery_led, battery_led_listener);
ZMK_SUBSCRIPTION(battery_led, zmk_keycode_state_changed);

/* 初期化 */
static int battery_led_init(void) {
    if (!device_is_ready(led_red.port)) return -ENODEV;
    if (!device_is_ready(led_green.port)) return -ENODEV;
    if (!device_is_ready(led_blue.port)) return -ENODEV;

    gpio_pin_configure_dt(&led_red, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led_green, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led_blue, GPIO_OUTPUT_INACTIVE);

    /* 最初は消しておく */
    all_leds_off();

    k_work_init_delayable(&led_off_work, led_off_handler);
    return 0;
}

SYS_INIT(battery_led_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
