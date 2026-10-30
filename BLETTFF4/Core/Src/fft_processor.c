#include "fft_processor.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

#define PI_F 3.14159265358979323846f
#define FFT_LENGTH 64

typedef struct {
    float real;
    float imag;
} Complex;

/* 位逆序重排 */
static void bit_reverse(Complex *a, int n)
{
    int j = 0;
    for (int i = 1; i < n; i++) {
        int bit = n >> 1;
        while (j >= bit) {
            j -= bit;
            bit >>= 1;
        }
        j += bit;
        if (i < j) {
            Complex temp = a[i];
            a[i] = a[j];
            a[j] = temp;
        }
    }
}

/* 迭代 FFT（Cooley-Tukey 蝶形运算） */
static void fft_radix2(Complex *a, int n)
{
    bit_reverse(a, n);
    for (int len = 2; len <= n; len <<= 1) {
        float angle = -2.0f * PI_F / len;
        float wlen_r = cosf(angle);
        float wlen_i = sinf(angle);
        for (int i = 0; i < n; i += len) {
            float w_r = 1.0f;
            float w_i = 0.0f;
            for (int j = 0; j < len / 2; j++) {
                float u_r = a[i + j].real;
                float u_i = a[i + j].imag;
                float v_r = w_r * a[i + j + len/2].real - w_i * a[i + j + len/2].imag;
                float v_i = w_r * a[i + j + len/2].imag + w_i * a[i + j + len/2].real;
                a[i + j].real = u_r + v_r;
                a[i + j].imag = u_i + v_i;
                a[i + j + len/2].real = u_r - v_r;
                a[i + j + len/2].imag = u_i - v_i;
                float next_w_r = w_r * wlen_r - w_i * wlen_i;
                w_i = w_r * wlen_i + w_i * wlen_r;
                w_r = next_w_r;
            }
        }
    }
}

void FFT_ProcessAndSend(UART_HandleTypeDef *huart, int *data, uint16_t length)
{
    Complex buf[FFT_LENGTH];
    float magnitude[FFT_LENGTH / 2];
    char tx_buf[32];
    uint16_t str_len;
    int i;

    /* 1. 整数输入 → float 复数 */
    for (i = 0; i < length && i < FFT_LENGTH; i++) {
        buf[i].real = (float)data[i];
        buf[i].imag = 0.0f;
    }
    for (; i < FFT_LENGTH; i++) {
        buf[i].real = 0.0f;
        buf[i].imag = 0.0f;
    }

    /* 2. 去直流（减均值），避免第 0 根谱线溢出 */
    float sum = 0.0f;
    for (i = 0; i < FFT_LENGTH; i++) {
        sum += buf[i].real;
    }
    float mean = sum / FFT_LENGTH;
    for (i = 0; i < FFT_LENGTH; i++) {
        buf[i].real -= mean;
    }

    /* 3. FFT */
    fft_radix2(buf, FFT_LENGTH);

    /* 4. 幅度谱（取前 N/2 = 32 根） */
    for (i = 0; i < FFT_LENGTH / 2; i++) {
        float r = buf[i].real;
        float im = buf[i].imag;
        magnitude[i] = sqrtf(r * r + im * im);
    }

    /* 5. 映射 0~330 并发送 [p,数值] */
    for (i = 0; i < FFT_LENGTH / 2; i++) {
        int mapped = (int)(magnitude[i] * 330.0f / 65504.0f);
        if (mapped > 330) mapped = 330;
        if (mapped < 0)   mapped = 0;

        str_len = sprintf(tx_buf, "[p,%d]", mapped);
        HAL_UART_Transmit(huart, (uint8_t *)tx_buf, str_len, HAL_MAX_DELAY);
        HAL_Delay(2);
    }
}
