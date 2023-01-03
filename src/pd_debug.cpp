//
// USB Power Delivery Sink Using FUSB302B
// Copyright (c) 2020 Manuel Bleichenbacher
//
// Licensed under MIT License
// https://opensource.org/licenses/MIT
//
// Debugging support (sending debugging messages to output)
//

#include "pd_debug.h"

#if defined(PD_DEBUG)

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include <stdio.h>
#include <string.h>

#include <algorithm>

namespace usb_pd {

constexpr int uart_tx_buf_len = 512;

// Buffer for data to be transmitted via UART
//  *  0 <= head < buf_len
//  *  0 <= tail < buf_len
//  *  head == tail => empty
//  *  head + 1 == tail => full (modulo uart_tx_buf_len)
// `tx_buf_head` points to the positions where the next character
// should be inserted. `tx_buf_tail` points to the character after
// the last character that has been transmitted.
static uint8_t tx_buf[uart_tx_buf_len];
static volatile int tx_buf_head;
static volatile int tx_buf_tail;

// The number of bytes currently being transmitted
static volatile int tx_size;

static char format_buf[80];

static void uart_set_baudrate(int baudrate) {
    usart_disable(USART1);
    usart_set_baudrate(USART1, baudrate);
    usart_set_databits(USART1, 8);
    usart_set_stopbits(USART1, USART_STOPBITS_1);
    usart_set_parity(USART1, USART_PARITY_NONE);
    usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
    usart_enable(USART1);
}

static void uart_init(int baudrate) {
    tx_buf_head = tx_buf_tail = 0;
    tx_size = 0;

    // Enable USART1 interface clock
    rcc_periph_clock_enable(RCC_USART1);

    // Enable TX pin clock
    rcc_periph_clock_enable(RCC_GPIOA);

    // Configure TX pin
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2);
    gpio_set_af(GPIOA, 1, GPIO2);

    // configure TX DMA (DMA 1 channel 2)
    rcc_periph_clock_enable(RCC_DMA1);

    // enable DMA interrupt (notifying about a completed transmission)
    nvic_set_priority(NVIC_DMA1_CHANNEL2_3_DMA2_CHANNEL1_2_IRQ, 2 << 6);
    nvic_enable_irq(NVIC_DMA1_CHANNEL2_3_DMA2_CHANNEL1_2_IRQ);

    dma_channel_reset(DMA1, DMA_CHANNEL2);
    dma_set_priority(DMA1, DMA_CHANNEL2, DMA_CCR_PL_LOW);

    dma_set_memory_size(DMA1, DMA_CHANNEL2, DMA_CCR_MSIZE_8BIT);
    dma_set_peripheral_size(DMA1, DMA_CHANNEL2, DMA_CCR_PSIZE_8BIT);
    dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL2);
    dma_set_read_from_memory(DMA1, DMA_CHANNEL2);

    dma_set_peripheral_address(DMA1, DMA_CHANNEL2, (uint32_t)&USART1_TDR);

    dma_enable_transfer_complete_interrupt(DMA1, DMA_CHANNEL2);

    usart_set_mode(USART1, USART_MODE_TX);
    uart_set_baudrate(baudrate);
    usart_enable(USART1);
}

static void uart_start_tx_dma(const uint8_t* buf, int len) {
    // set transmit chunk
    dma_set_memory_address(DMA1, DMA_CHANNEL2, (uint32_t)buf);
    dma_set_number_of_data(DMA1, DMA_CHANNEL2, len);

    // start transmission
    usart_enable_tx_dma(USART1);
    dma_enable_channel(DMA1, DMA_CHANNEL2);
}

static void uart_start_transmit() {
    int start_pos;

    if (tx_size != 0 || tx_buf_head == tx_buf_tail)
        return; // UART busy or queue empty

    // Determine TX chunk size
    start_pos = tx_buf_tail;
    int end_pos = tx_buf_head;
    if (end_pos <= start_pos)
        end_pos = uart_tx_buf_len;
    tx_size = end_pos - start_pos;
    if (tx_size > 32)
        tx_size = 32; // no more than 32 bytes to free up space soon

    uart_start_tx_dma(tx_buf + start_pos, tx_size);
}

static void uart_transmit(const uint8_t* data, size_t len) {
    size_t size;

    int buf_tail = tx_buf_tail;
    int buf_head = tx_buf_head;

    size_t avail_chunk_size;
    if (buf_head < buf_tail)
        avail_chunk_size = buf_tail - buf_head - 1;
    else if (buf_tail != 0)
        avail_chunk_size = uart_tx_buf_len - buf_head;
    else
        avail_chunk_size = uart_tx_buf_len - 1 - buf_head;

    if (avail_chunk_size == 0)
        return; // buffer full - discard data

    // Copy data to transmit buffer
    size = std::min(len, avail_chunk_size);
    memcpy(tx_buf + buf_head, data, size);
    buf_head += size;
    if (buf_head >= uart_tx_buf_len)
        buf_head = 0;
    tx_buf_head = buf_head;

    // start transmission
    uart_start_transmit();

    // Use second transmit in case of remaining data
    // (usually because of wrap around at the end of the buffer)
    if (size < len)
        uart_transmit(data + size, len - size);
}

static void uart_print(const char* str) {
    uart_transmit((const uint8_t*)str, strlen(str));
}

void uart_on_tx_complete() {
    // Update TX buffer
    int buf_tail = tx_buf_tail + tx_size;
    if (buf_tail >= uart_tx_buf_len)
        buf_tail = 0;
    tx_buf_tail = buf_tail;
    tx_size = 0;

    // Check for next transmission
    uart_start_transmit();
}

void debug_init() {
    uart_init(115200);
    uart_print("ZY12PDN OSS\r\n");
}

void debug_log(const char* msg, uint32_t val) {
    int len = snprintf(format_buf, sizeof(format_buf), msg, val);
    uart_transmit((const uint8_t*)format_buf, len);
}

} // namespace usb_pd

extern "C" void dma1_channel2_3_dma2_channel1_2_isr(void) {
    if (dma_get_interrupt_flag(DMA1, DMA_CHANNEL2, DMA_TCIF)) {
        // Disable DMA
        dma_disable_channel(DMA1, DMA_CHANNEL2);
        dma_clear_interrupt_flags(DMA1, DMA_CHANNEL2, DMA_TCIF);

        usb_pd::uart_on_tx_complete();
    }
}

#endif
