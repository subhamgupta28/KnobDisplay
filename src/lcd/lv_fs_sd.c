#include "lvgl.h"
#include <stdio.h>
#include <string.h>
#include "sd_card_bsp.h" // Ensure your SD is mounted to /sdcard

// -------- File API wrappers --------
static void *fs_open(lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode)
{
    const char *flags = "";
    if (mode == LV_FS_MODE_WR) flags = "wb";
    else if (mode == LV_FS_MODE_RD) flags = "rb";
    else if (mode == (LV_FS_MODE_WR | LV_FS_MODE_RD)) flags = "rb+";

    static char full_path[128];
    snprintf(full_path, sizeof(full_path), "%s", path); // Map LVGL 'S:' to /sdcard

    FILE *f = fopen(full_path, flags);
    if (!f) {
        printf("❌ fs_open failed: %s\n", full_path);
    } else {
        printf("✅ fs_open OK: %s\n", full_path);
    }
    return f;
}

static lv_fs_res_t fs_close(lv_fs_drv_t *drv, void *file_p)
{
    if (file_p) fclose((FILE *)file_p);
    return LV_FS_RES_OK;
}

static lv_fs_res_t fs_read(lv_fs_drv_t *drv, void *file_p, void *buf, uint32_t btr, uint32_t *br)
{
    if (!file_p) return LV_FS_RES_UNKNOWN;
    *br = fread(buf, 1, btr, (FILE *)file_p);
    return LV_FS_RES_OK;
}

static lv_fs_res_t fs_seek(lv_fs_drv_t *drv, void *file_p, uint32_t pos, lv_fs_whence_t whence)
{
    if (!file_p) return LV_FS_RES_UNKNOWN;
    int w = SEEK_SET;
    if (whence == LV_FS_SEEK_CUR) w = SEEK_CUR;
    else if (whence == LV_FS_SEEK_END) w = SEEK_END;
    fseek((FILE *)file_p, pos, w);
    return LV_FS_RES_OK;
}

static lv_fs_res_t fs_tell(lv_fs_drv_t *drv, void *file_p, uint32_t *pos)
{
    if (!file_p) return LV_FS_RES_UNKNOWN;
    *pos = ftell((FILE *)file_p);
    return LV_FS_RES_OK;
}

// -------- Directory API wrappers (minimal support) --------
static void *fs_dir_open(lv_fs_drv_t *drv, const char *path)
{
    // Optional: implement directory listing if needed
    return NULL;
}

static lv_fs_res_t fs_dir_read(lv_fs_drv_t *drv, void *rddir_p, char *fn)
{
    if (fn) fn[0] = '\0';
    return LV_FS_RES_OK;
}

static lv_fs_res_t fs_dir_close(lv_fs_drv_t *drv, void *rddir_p)
{
    return LV_FS_RES_OK;
}

// -------- Register the SD filesystem driver with LVGL --------
void lv_fs_sd_init(void)
{
    static lv_fs_drv_t fs_drv;
    lv_fs_drv_init(&fs_drv);

    fs_drv.letter = 'S';          // Drive letter
    fs_drv.open_cb = fs_open;
    fs_drv.close_cb = fs_close;
    fs_drv.read_cb = fs_read;
    fs_drv.seek_cb = fs_seek;
    fs_drv.tell_cb = fs_tell;
    fs_drv.dir_close_cb = fs_dir_close;
    fs_drv.dir_open_cb = fs_dir_open;
    fs_drv.dir_read_cb = fs_dir_read;

    lv_fs_drv_register(&fs_drv);

    printf("LVGL SD driver registered: S: -> /sdcard\n");
}
