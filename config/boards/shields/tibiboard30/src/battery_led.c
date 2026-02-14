/* config/src/battery_led.c */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>

/* LED機能を無効化するためのダミー関数 */
static int battery_led_init(const struct device *dev) {
    return 0;
}

/* 何もしない初期化（ビルドエラー回避用） */
SYS_INIT(battery_led_init, APPLICATION, 90);
