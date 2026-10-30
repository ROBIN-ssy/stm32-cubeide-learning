#ifndef __WAVE_PLOTTER_H
#define __WAVE_PLOTTER_H

#include "stdint.h"
#include "main.h"   /* 提供 UART_HandleTypeDef 等 HAL 类型定义 */

/*
 * @brief  发送单通道波形数据 (带电压映射)
 * @param  huart  目标串口句柄指针 (例如 &huart3)
 * @param  data   波形数据数组指针
 * @param  length 数据的长度
 */
void Plotter_Send_1CH(UART_HandleTypeDef *huart, int *data, uint16_t length);

/*
 * @brief  发送双通道波形数据 (带电压映射)
 * @param  huart  目标串口句柄指针 (例如 &huart3)
 * @param  data1  通道1波形数据指针
 * @param  data2  通道2波形数据指针
 * @param  length 数据的长度
 */
void Plotter_Send_2CH(UART_HandleTypeDef *huart, int *data1, int *data2, uint16_t length);

#endif /* __WAVE_PLOTTER_H */
