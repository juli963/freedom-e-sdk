/*
 *  Copyright (c) 2018 Bastian Koppelmann Paderborn Univeristy
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include "platform.h"
#include "spi.h"

#define BIT(x) (1<<(x))
#define IDLE asm volatile ("")

#define SPI_MODE0 0x00
#define SPI_REG(x) SPI1_REG(x)

static const uint32_t SPI_IOF_MASK = (1 << IOF_SPI1_SCK) | (1 << IOF_SPI1_MOSI) | (1 << IOF_SPI1_MISO);

SPIMaster *spi_dev;

typedef enum {
    SPI_CONTINUE,
    SPI_LAST
} SPITransferMode;

typedef enum {
    LSBFIRST,
    MSBFIRST
} BitOrder;

static void cwait(uint32_t cycle_delay)
{
    volatile uint32_t i;
    for (i = 0; i < cycle_delay; ++i);
}

void spi_setClockDivider(uint8_t _divider) {
  SPI_REG(SPI_REG_SCKDIV) = _divider;
}

void spi_setDataMode(uint8_t _mode) {
  SPI_REG(SPI_REG_SCKMODE) = _mode;
}

void spi_setBitOrder(BitOrder _bitOrder) {
  SPI_REG(SPI_REG_FMT) = SPI_FMT_PROTO(SPI_PROTO_S) |
    SPI_FMT_ENDIAN((_bitOrder == LSBFIRST) ? SPI_ENDIAN_LSB : SPI_ENDIAN_MSB) |
    SPI_FMT_DIR(SPI_DIR_RX) |
    SPI_FMT_LEN(8);
}

uint8_t spi_transfer(uint8_t _data, SPITransferMode _mode) {

  while (SPI_REG(SPI_REG_TXFIFO) & SPI_TXFIFO_FULL) ;
  SPI_REG(SPI_REG_TXFIFO) = _data;

  volatile int32_t x;
  while ((x = SPI_REG(SPI_REG_RXFIFO)) & SPI_RXFIFO_EMPTY);
  return x & 0xFF;

  if (_mode == SPI_LAST) {
    SPI_REG(SPI_REG_CSMODE) = SPI_CSMODE_AUTO;
  }
}

int8_t spi_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data,
                 uint16_t len)
{
    GPIO_REG(GPIO_OUTPUT_VAL) &= ~BIT(spi_dev->cs);
    spi_transfer(reg_addr, SPI_CONTINUE);
    for (int i = 0; i < len-1; ++i) {
        spi_transfer(data[i], SPI_CONTINUE);
    }
    spi_transfer(data[len-1], SPI_LAST);
    GPIO_REG(GPIO_OUTPUT_VAL) |= BIT(spi_dev->cs);
    return 0;
}

int8_t spi_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data,
                uint16_t len)
{
    uint8_t result;
    GPIO_REG(GPIO_OUTPUT_VAL) &= ~BIT(spi_dev->cs);
    spi_transfer(reg_addr, SPI_CONTINUE);

    for (int i = 0; i < len-1; ++i) {
        data[i] = spi_transfer(reg_addr, SPI_CONTINUE);
    }
    data[len-1] = spi_transfer(reg_addr, SPI_LAST);
    GPIO_REG(GPIO_OUTPUT_VAL) |= BIT(spi_dev->cs);
    return 0;
}


void spi_begin(uint32_t cs, uint32_t max_slave_freq)
{
    spi_dev = malloc(sizeof(SPIMaster));
    spi_dev->cs = cs;

    GPIO_REG(GPIO_IOF_SEL) &= ~SPI_IOF_MASK;
    GPIO_REG(GPIO_IOF_EN)  |= SPI_IOF_MASK;

    spi_setDataMode(SPI_MODE0);
    spi_setBitOrder(MSBFIRST);

    GPIO_REG(GPIO_OUTPUT_EN) |= BIT(cs);
    spi_setClockDivider((65000000/ max_slave_freq) - 1);
    GPIO_REG(GPIO_OUTPUT_VAL) |= BIT(cs);
    cwait(15000);

}
