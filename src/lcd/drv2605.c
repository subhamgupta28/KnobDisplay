#include "driver/i2c.h"
#include "esp_log.h"

#define DRV2605_ADDR   0x5A   // DRV2605 I2C address
#define I2C_PORT       I2C_NUM_1

static const char *TAG = "DRV2605";

// Write 1 byte to a register
esp_err_t drv2605_write_reg(uint8_t reg, uint8_t val)
{
    uint8_t data[2] = {reg, val};
    return i2c_master_write_to_device(I2C_PORT, DRV2605_ADDR, data, 2, 1000 / portTICK_PERIOD_MS);
}

// Read 1 byte from a register
esp_err_t drv2605_read_reg(uint8_t reg, uint8_t *val)
{
    return i2c_master_write_read_device(I2C_PORT, DRV2605_ADDR, &reg, 1, val, 1, 1000 / portTICK_PERIOD_MS);
}

// Initialize DRV2605
esp_err_t drv2605_init(void)
{
    esp_err_t ret;
    uint8_t id;

    // Read status / device ID (Reg 0x00)
    ret = drv2605_read_reg(0x00, &id);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read DRV2605 ID");
        return ret;
    }

    ESP_LOGI(TAG, "DRV2605 status/ID: 0x%02X", id);

    // Mode register (0x01) → set to internal trigger
    ret = drv2605_write_reg(0x01, 0x00); // MODE_INTTRIG
    if (ret != ESP_OK) return ret;

    // Library selection (0x03) → library 1
    ret = drv2605_write_reg(0x03, 1);
    if (ret != ESP_OK) return ret;

    ESP_LOGI(TAG, "DRV2605 init success");
    return ESP_OK;
}

// Play an effect
esp_err_t drv2605_play_effect(uint8_t effect)
{
    // Register 0x04 = waveform sequence
    esp_err_t ret = drv2605_write_reg(0x04, effect);
    if (ret != ESP_OK) return ret;

    // End of sequence marker
    ret = drv2605_write_reg(0x05, 0);
    if (ret != ESP_OK) return ret;

    // Go register (0x0C) = 1 → start playback
    return drv2605_write_reg(0x0C, 1);
}
