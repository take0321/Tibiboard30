#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zmk/battery.h>

/* XIAO BLEのLED定義 */
static const struct gpio_dt_spec led_red   = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec led_green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec led_blue  = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

/* 表示時間 (2秒) */
#define LED_SHOW_TIME 2000

static struct k_work_delayable led_off_work;
static struct k_work_delayable boot_show_work;

/* LED消灯 */
static void all_leds_off(void) {
    if (device_is_ready(led_red.port)) gpio_pin_set_dt(&led_red, 1);
    if (device_is_ready(led_green.port)) gpio_pin_set_dt(&led_green, 1);
    if (device_is_ready(led_blue.port)) gpio_pin_set_dt(&led_blue, 1);
}

static void led_off_handler(struct k_work *work) {
    all_leds_off();
}

/* バッテリー表示のメイン処理 */
static void show_battery_level(void) {
    uint8_t level = zmk_battery_state_of_charge();
    
    k_work_cancel_delayable(&led_off_work);
    all_leds_off();

    /* 色の決定 */
    if (level >= 80) {
        gpio_pin_set_dt(&led_green, 0); /* 緑 */
    } else if (level >= 30) {
        gpio_pin_set_dt(&led_blue, 0);  /* 青 */
    } else {
        gpio_pin_set_dt(&led_red, 0);   /* 赤 */
    }

    /* 2秒後に消す */
    k_work_schedule(&led_off_work, K_MSEC(LED_SHOW_TIME));
}

/* 起動時の呼び出し用 */
static void boot_show_handler(struct k_work *work) {
    show_battery_level();
}

/* 初期化処理 */
static int battery_led_init(void) {
    if (!device_is_ready(led_red.port)) return -ENODEV;
    
    gpio_pin_configure_dt(&led_red, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led_green, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led_blue, GPIO_OUTPUT_INACTIVE);

    all_leds_off();
    
    k_work_init_delayable(&led_off_work, led_off_handler);
    k_work_init_delayable(&boot_show_work, boot_show_handler);

    /* 起動してから 1秒後 に表示する */
    k_work_schedule(&boot_show_work, K_MSEC(1000));

    return 0;
}

SYS_INIT(battery_led_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
