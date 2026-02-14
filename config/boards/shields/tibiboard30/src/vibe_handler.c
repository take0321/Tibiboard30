/* config/src/vibe_handler.c */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>

/* 振動機能を無効化するためのダミー関数 */
static int vibration_init(const struct device *dev) {
    return 0;
}

/* 何もしない初期化関数を登録（ビルドエラー回避用） */
SYS_INIT(vibration_init, POST_KERNEL, 90);
