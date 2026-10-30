/**
  ******************************************************************************
  * @file    oled.c
  * @brief   OLED SSD1306 驱动实现 (128x64, I2C)
  *          适用于 STM32F103C8T6 + HAL库
  ******************************************************************************
  * @attention
  *  显存模型: 128列 x 64行, 8个page, 共1024字节
  *  取模方式: 纵向取模, 字节倒序 (LSB在上, MSB在下)
  *  I2C地址: 0x78 (7位地址 0x3C 左移1位)
  *  依赖: hi2c1 (在 i2c.c 中定义), font.h (字模数据)
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "oled.h"
#include "font.h"
#include "i2c.h"
#include <string.h>

/* Private defines -----------------------------------------------------------*/

/* SSD1306 命令 */
#define SSD1306_SET_CONTRAST          0x81
#define SSD1306_DISPLAY_ON            0xAF
#define SSD1306_DISPLAY_OFF           0xAE
#define SSD1306_NORMAL_DISPLAY        0xA6
#define SSD1306_INVERT_DISPLAY        0xA7
#define SSD1306_DISPLAY_RAM           0xA4
#define SSD1306_DISPLAY_ALLON         0xA5
#define SSD1306_SET_MEM_ADDR_MODE     0x20
#define SSD1306_SET_COL_ADDR          0x21
#define SSD1306_SET_PAGE_ADDR         0x22
#define SSD1306_SET_START_LINE       0x40  /* | line (0-63) */
#define SSD1306_SET_REMAP_COL        0xA1  /* 列地址映射: A0=正常, A1=反转 */
#define SSD1306_SET_REMAP_ROW        0xC8  /* 行地址映射: C0=正常, C8=反转 */
#define SSD1306_SET_MUX_RATIO        0xA8
#define SSD1306_SET_DISPLAY_OFFSET   0xD3
#define SSD1306_SET_COMPINS          0xDA
#define SSD1306_SET_PRECHARGE        0xD9
#define SSD1306_SET_VCOMH            0xDB
#define SSD1306_CHARGE_PUMP          0x8D
#define SSD1306_SET_CLK_DIV          0xD5
#define SSD1306_SET_SEG_REMAP_OFF    0xA0
#define SSD1306_SET_COM_SCAN_DIR_OFF 0xC0

/* Private variables ---------------------------------------------------------*/
static uint8_t OLED_FrameBuffer[OLED_WIDTH * OLED_PAGE_NUM]; /* 1024字节显存 */

/* External variables --------------------------------------------------------*/
extern I2C_HandleTypeDef hi2c1;

/* Private function prototypes -----------------------------------------------*/
static void OLED_WriteCommand(uint8_t cmd);
static void OLED_WriteData(uint8_t *data, uint16_t len);
static void OLED_WriteByte(uint8_t byte);

/* ===================== 底层 I2C 通信 ===================== */

/**
 * @brief  通过 I2C 写一条命令到 SSD1306
 */
static void OLED_WriteCommand(uint8_t cmd)
{
    uint8_t buf[2] = {OLED_CMD, cmd};
    HAL_I2C_Master_Transmit(&hi2c1, OLED_I2C_ADDR, buf, 2, 100);
}

/**
 * @brief  通过 I2C 写数据到 SSD1306
 * @param  data  数据指针
 * @param  len   数据长度
 * @note   使用 static 缓冲区避免在栈上分配大数组 (栈仅 1KB)
 */
static void OLED_WriteData(uint8_t *data, uint16_t len)
{
    /* SSD1306 要求先发控制字节 0x40, 再发数据 */
    static uint8_t buf[OLED_WIDTH * OLED_PAGE_NUM + 1]; /* 1025字节, 放在 BSS 段 */
    buf[0] = OLED_DATA;
    memcpy(&buf[1], data, len);
    HAL_I2C_Master_Transmit(&hi2c1, OLED_I2C_ADDR, buf, len + 1, 200);
}

/**
 * @brief  通过 I2C 写一个字节的数据到 SSD1306
 */
static void OLED_WriteByte(uint8_t byte)
{
    uint8_t buf[2] = {OLED_DATA, byte};
    HAL_I2C_Master_Transmit(&hi2c1, OLED_I2C_ADDR, buf, 2, 100);
}

/* ===================== SSD1306 初始化 ===================== */

/**
 * @brief  初始化 OLED (SSD1306 128x64)
 */
void OLED_Init(void)
{
    /* 等待屏幕稳定 */
    HAL_Delay(100);

    /* 关闭显示 */
    OLED_WriteCommand(SSD1306_DISPLAY_OFF);

    /* 设置时钟分频和振荡频率 */
    OLED_WriteCommand(SSD1306_SET_CLK_DIV);
    OLED_WriteCommand(0x80);  /* 建议值 */

    /* 设置 MUX 比率 */
    OLED_WriteCommand(SSD1306_SET_MUX_RATIO);
    OLED_WriteCommand(0x3F);  /* 1/64 duty */

    /* 设置显示偏移 */
    OLED_WriteCommand(SSD1306_SET_DISPLAY_OFFSET);
    OLED_WriteCommand(0x00);

    /* 设置起始行 */
    OLED_WriteCommand(SSD1306_SET_START_LINE | 0x00);

    /* 电荷泵: 开启 (OLED 需要内部升压) */
    OLED_WriteCommand(SSD1306_CHARGE_PUMP);
    OLED_WriteCommand(0x14);

    /* 列地址映射: A1 = 从左到右 */
    OLED_WriteCommand(SSD1306_SET_REMAP_COL);

    /* 行地址映射: C8 = 从上到下 */
    OLED_WriteCommand(SSD1306_SET_REMAP_ROW);

    /* 设置 COM 引脚硬件配置 */
    OLED_WriteCommand(SSD1306_SET_COMPINS);
    OLED_WriteCommand(0x12);  /* 128x64 用 0x12 */

    /* 设置对比度 */
    OLED_WriteCommand(SSD1306_SET_CONTRAST);
    OLED_WriteCommand(0xCF);  /* 最高对比度 */

    /* 设置预充电周期 */
    OLED_WriteCommand(SSD1306_SET_PRECHARGE);
    OLED_WriteCommand(0xF1);

    /* 设置 VCOMH 电压 */
    OLED_WriteCommand(SSD1306_SET_VCOMH);
    OLED_WriteCommand(0x40);

    /* 设置内存寻址模式: 水平寻址模式 */
    OLED_WriteCommand(SSD1306_SET_MEM_ADDR_MODE);
    OLED_WriteCommand(0x00);  /* 0=水平, 1=垂直, 2=页 */

    /* 正常显示 (非反色) */
    OLED_WriteCommand(SSD1306_NORMAL_DISPLAY);

    /* 显示 RAM 内容 (非全亮) */
    OLED_WriteCommand(SSD1306_DISPLAY_RAM);

    /* 清屏 */
    OLED_NewFrame();
    OLED_ShowFrame();

    /* 开启显示 */
    OLED_WriteCommand(SSD1306_DISPLAY_ON);
}

/* ===================== 显存操作 ===================== */

/**
 * @brief  清空显存 (全黑)
 */
void OLED_NewFrame(void)
{
    memset(OLED_FrameBuffer, 0, sizeof(OLED_FrameBuffer));
}

/**
 * @brief  填充整个屏幕
 */
void OLED_Fill(OLED_Color_t color)
{
    if (color == OLED_COLOR_NORMAL)
        memset(OLED_FrameBuffer, 0xFF, sizeof(OLED_FrameBuffer));
    else
        memset(OLED_FrameBuffer, 0x00, sizeof(OLED_FrameBuffer));
}

/**
 * @brief  在显存中画一个点
 * @param  x     列坐标 (0-127)
 * @param  y     行坐标 (0-63)
 * @param  color 颜色
 */
void OLED_DrawPoint(uint8_t x, uint8_t y, OLED_Color_t color)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT)
        return;

    uint16_t index = x + (y / 8) * OLED_WIDTH;
    uint8_t bit_mask = 1 << (y % 8);

    if (color == OLED_COLOR_NORMAL)
        OLED_FrameBuffer[index] |= bit_mask;    /* 置1 = 亮 */
    else
        OLED_FrameBuffer[index] &= ~bit_mask;   /* 清0 = 暗 */
}

/**
 * @brief  刷新显存到屏幕
 *         一次性传输整个 1024 字节
 */
void OLED_ShowFrame(void)
{
    /* 设置列地址范围: 0-127 */
    OLED_WriteCommand(SSD1306_SET_COL_ADDR);
    OLED_WriteCommand(0x00);
    OLED_WriteCommand(OLED_WIDTH - 1);

    /* 设置页地址范围: 0-7 */
    OLED_WriteCommand(SSD1306_SET_PAGE_ADDR);
    OLED_WriteCommand(0x00);
    OLED_WriteCommand(OLED_PAGE_NUM - 1);

    /* 传输整个显存 */
    OLED_WriteData(OLED_FrameBuffer, sizeof(OLED_FrameBuffer));
}

/* ===================== 绘图函数 ===================== */

/**
 * @brief  绘制图片
 * @param  x, y   左上角坐标
 * @param  img     图片指针 (data为纵向取模数据)
 * @param  color   颜色
 *
 * 图片数据格式: 纵向取模, 字节倒序
 *   每列 ceil(h/8) 字节, 每字节的bit0在上(LSB)
 *   与显存格式完全一致
 */
void OLED_DrawImage(uint8_t x, uint8_t y, const OLED_Image_t *img, OLED_Color_t color)
{
    if (img == NULL || img->data == NULL)
        return;

    uint8_t bytes_per_col = (img->h + 7) / 8;

    for (uint8_t col = 0; col < img->w; col++)
    {
        int8_t sx = x + col;
        if (sx < 0 || sx >= OLED_WIDTH)
            continue;

        for (uint8_t byte_idx = 0; byte_idx < bytes_per_col; byte_idx++)
        {
            uint8_t byte_val = img->data[col * bytes_per_col + byte_idx];

            for (uint8_t bit = 0; bit < 8; bit++)
            {
                int8_t sy = y + byte_idx * 8 + bit;
                if (sy < 0 || sy >= OLED_HEIGHT)
                    continue;

                if (byte_val & (1 << bit))
                    OLED_DrawPoint(sx, sy, color);
            }
        }
    }
}

/* ===================== 字符串打印 ===================== */

/**
 * @brief  UTF-8 解码: 获取一个 UTF-8 字符的字节数和 Unicode 码点
 * @param  str    字符串指针
 * @param  bytes   返回该字符占用的字节数
 * @return Unicode 码点
 */
static uint32_t utf8_decode(const char *str, uint8_t *bytes)
{
    uint8_t c = (uint8_t)str[0];
    
    if (c < 0x80)
    {
        /* ASCII: 1字节 */
        *bytes = 1;
        return c;
    }
    else if ((c & 0xE0) == 0xC0)
    {
        /* 2字节 UTF-8 */
        *bytes = 2;
        return ((c & 0x1F) << 6) | ((uint8_t)str[1] & 0x3F);
    }
    else if ((c & 0xF0) == 0xE0)
    {
        /* 3字节 UTF-8 (中文) */
        *bytes = 3;
        return ((c & 0x0F) << 12) | (((uint8_t)str[1] & 0x3F) << 6) | ((uint8_t)str[2] & 0x3F);
    }
    else
    {
        /* 4字节 UTF-8 (不常用, 简化处理) */
        *bytes = 4;
        return 0;
    }
}

/**
 * @brief  在中文字模表中查找指定字符的索引
 * @param  str  UTF-8字符串(已知为3字节中文)
 * @return 字模数据中的索引, -1表示未找到
 */
static int16_t find_cn_index(const char *str, const OLED_Font_t *font)
{
    /* 遍历 cn_str 中的中文(每个3字节), 比较前3个字节 */
    const char *p = font->cn_str;
    uint16_t idx = 0;

    while (*p)
    {
        if (memcmp(p, str, 3) == 0)
            return idx;
        p += 3;
        idx++;
    }
    return -1;
}

/**
 * @brief  打印字符串 (支持中英文混合, UTF-8编码)
 *
 * 字模数据格式:
 *   - 中文: 纵向取模, 16x16, 每字32字节 (16列 x 2字节)
 *   - ASCII: 纵向取模, 8x16, 每字16字节 (8列 x 1字节)
 *
 * @param  x, y   左上角坐标
 * @param  str     UTF-8字符串
 * @param  font    字体
 * @param  color   颜色
 */
void OLED_PrintString(uint8_t x, uint8_t y, const char *str, const OLED_Font_t *font, OLED_Color_t color)
{
    if (str == NULL || font == NULL)
        return;

    uint8_t cx = x;

    while (*str)
    {
        uint8_t bytes;
        uint32_t code = utf8_decode(str, &bytes);

        if (bytes == 3)
        {
            /* 中文字符 */
            int16_t cn_idx = find_cn_index(str, font);
            if (cn_idx >= 0)
            {
                /* 字模数据: 每个中文 font->w * (font->h/8) 字节 */
                uint16_t char_bytes = font->w * ((font->h + 7) / 8);
                const uint8_t *char_data = font->cn_data + cn_idx * char_bytes;

                /* 构造临时 Image 结构体来复用 DrawImage 逻辑 */
                OLED_Image_t tmp = {font->w, font->h, char_data};
                OLED_DrawImage(cx, y, &tmp, color);
            }
            cx += font->w;
        }
        else if (bytes == 1)
        {
            /* ASCII字符 */
            if (code >= 0x20 && code <= 0x7E)
            {
                uint16_t asc_idx = code - 0x20;
                uint8_t asc_w = font->w / 2;  /* ASCII宽度=中文宽度的一半 */
                uint16_t char_bytes = asc_w * ((font->h + 7) / 8);
                const uint8_t *char_data = font->asc_data + asc_idx * char_bytes;

                OLED_Image_t tmp = {asc_w, font->h, char_data};
                OLED_DrawImage(cx, y, &tmp, color);
            }
            cx += font->w / 2;
        }

        str += bytes;
    }
}

/* ===================== 其他控制 ===================== */

/**
 * @brief  开启/关闭 OLED 显示
 * @param  on  1=开启, 0=关闭
 */
void OLED_DisplayOn(uint8_t on)
{
    if (on)
        OLED_WriteCommand(SSD1306_DISPLAY_ON);
    else
        OLED_WriteCommand(SSD1306_DISPLAY_OFF);
}
