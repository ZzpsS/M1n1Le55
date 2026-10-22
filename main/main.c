#include "bsp/esp-bsp.h"
#include "fm1_host.h"
#include "fm1_storage.h"
#include "fm1_samples.h"
#include "fm1_update.h"
#include "fm1_ble.h"
#include "fm1_adapter.h"
#include "esp_log.h"

void fm1_ui_start(void);
void fm1_diag_start(void);

void app_main(void)
{
    // The default BSP allocates two draw buffers and a rotation buffer in
    // internal DMA RAM (~338 KiB). P4's PPA/DSI drivers support PSRAM buffers
    // and handle cache synchronization, leaving internal RAM for USB and BLE.
    const bsp_display_cfg_t display_config={
        .lvgl_port_cfg=ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size=BSP_LCD_H_RES*CONFIG_BSP_LCD_DRAW_BUF_HEIGHT,
        .double_buffer=true,
        .flags={.buff_dma=false,.buff_spiram=true,.sw_rotate=true}
    };
    lv_display_t *display = bsp_display_start_with_config(&display_config);
    if (!display) { ESP_LOGE("tab5_fm1", "Display initialization failed"); return; }
    bsp_display_backlight_on();
    bsp_display_lock(0);
    bsp_display_rotate(display, LV_DISPLAY_ROTATION_90);
    fm1_ui_start();
    bsp_display_unlock();
    ESP_ERROR_CHECK(fm1_host_start());
    fm1_adapter_start();
    fm1_storage_start();
    fm1_update_start();
    fm1_ble_start();
    fm1_samples_start();
    fm1_diag_start();
    ESP_LOGI("tab5_fm1", "Editor ready: %ld x %ld", (long)lv_display_get_horizontal_resolution(display),
             (long)lv_display_get_vertical_resolution(display));
}
