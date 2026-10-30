/*
 * fft_processor.h
 *
 *  Created on: Jul 19, 2026
 *      Author: 24431
 */
#ifndef __FFT_PROCESSOR_H
#define __FFT_PROCESSOR_H

#include "main.h"

#define FFT_LENGTH 64    /* FFT 点数，和正弦表点数一致 */

/*
 * @brief  对输入数据做 64 点实数 FFT，计算幅度谱，
 *         以 [p,数值] 格式通过串口发送（共 32 个频谱点）
 * @param  huart  目标串口句柄
 * @param  data   输入数据（整数数组，如 sineTable）
 * @param  length 数据长度（应等于 FFT_LENGTH）
 */
void FFT_ProcessAndSend(UART_HandleTypeDef *huart, int *data, uint16_t length);

#endif /* __FFT_PROCESSOR_H */
