#ifndef SD_CARD_BSP_H
#define SD_CARD_BSP_H

void sd_card_Init(void);
float sd_card_get_value(void);
esp_err_t s_example_write_file(const char *path, char *data);
esp_err_t s_example_read_file(const char *path,char *pxbuf,uint32_t *outLen);
// In sd_card_bsp.h
esp_err_t sd_card_list_files(const char *dirPath, uint8_t levels);
esp_err_t sd_card_delete_file(const char *path);

#endif