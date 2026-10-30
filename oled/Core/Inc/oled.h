/**
  ******************************************************************************
  * @file    oled.h
  * @brief   OLED SSD1306 驱动头文件 (128x64, I2C)
  *          基于 HAL I2C, 适用于 STM32F103C8T6
  ******************************************************************************
  * @attention
  *  显存模型: 128列 x 64行, 8个page, 每page 128字节, 共1024字节
  *  取模方式: 纵向取模, 字节倒序 (LSB在上, MSB在下)
  *  I2C地址: 0x78 (7位地址 0x3C 左移1位)
  ******************************************************************************
  */

#ifndef __OLED_H
#define __OLED_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"
#include <stdint.h>

/* ===================== 宏定义 ===================== */
#define OLED_WIDTH      128
#define OLED_HEIGHT     64
#define OLED_PAGE_NUM   8          /* 64/8 = 8 pages */

#define OLED_I2C_ADDR   0x78       /* 7位地址 0x3C 左移1位 */

/* I2C 控制字节 */
#define OLED_CMD        0x00       /* Co=0, D/C=0 → 命令 */
#define OLED_DATA       0x40       /* Co=0, D/C=1 → 数据 */

/* ===================== 颜色枚举 ===================== */
typedef enum {
    OLED_COLOR_NORMAL  = 0,        /* 正常显示: 像素亮 */
    OLED_COLOR_REVERSED = 1,       /* 反色显示: 像素暗 */
} OLED_Color_t;

/* ===================== 图片结构体 ===================== */
typedef struct {
    const uint8_t w;               /* 图片宽度 (像素) */
    const uint8_t h;               /* 图片高度 (像素) */
    const uint8_t *data;           /* 图片数据指针 (纵向取模, 字节倒序) */
} OLED_Image_t;

/* ===================== 字体结构体 ===================== */
typedef struct {
    const uint8_t w;               /* 单字符宽度 (像素) */
    const uint8_t h;               /* 单字符高度 (像素) */
    const uint8_t *cn_data;        /* 中文字模数据 (按UTF-8编码顺序排列) */
    const char *cn_str;            /* 中文索引字符串 (UTF-8) */
    const uint8_t *asc_data;       /* ASCII字模数据 (从空格0x20开始) */
    const uint16_t asc_count;      /* ASCII字符数量 */
} OLED_Font_t;

/* ===================== 全局变量声明 ===================== */
extern const OLED_Image_t bilibiliImg;
extern const OLED_Font_t font16x16;

/* ===================== API 函数声明 ===================== */

/**
 * @brief  初始化 OLED (SSD1306)
 */
void OLED_Init(void);

/**
 * @brief  开启新帧, 清空显存
 */
void OLED_NewFrame(void);

/**
 * @brief  刷新显存到屏幕
 */
void OLED_ShowFrame(void);

/**
 * @brief  在显存中画一个点
 * @param  x, y  坐标
 * @param  color 颜色 (NORMAL=亮 / REVERSED=暗)
 */
void OLED_DrawPoint(uint8_t x, uint8_t y, OLED_Color_t color);

/**
 * @brief  绘制图片
 * @param  x, y   左上角坐标
 * @param  img     图片指针
 * @param  color   颜色
 */
void OLED_DrawImage(uint8_t x, uint8_t y, const OLED_Image_t *img, OLED_Color_t color);

/**
 * @brief  打印字符串 (支持中英文混合, UTF-8编码)
 * @param  x, y   左上角坐标
 * @param  str     UTF-8字符串
 * @param  font    字体
 * @param  color   颜色
 */
void OLED_PrintString(uint8_t x, uint8_t y, const char *str, const OLED_Font_t *font, OLED_Color_t color);

/**
 * @brief  填充整个屏幕
 * @param  color 颜色
 */
void OLED_Fill(OLED_Color_t color);

/**
 * @brief  开启/关闭 OLED 显示
 * @param  on  1=开启, 0=关闭
 */
void OLED_DisplayOn(uint8_t on);

#ifdef __cplusplus
}
#endif

#endif /* __OLED_H */
