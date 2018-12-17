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
#include <string.h>
#include <unistd.h>
#include "plic/plic_driver.h"
#include "platform.h"
#include "encoding.h"
#include "spi.h"
#include "common.h"
#include "UART_driver.h"
#include "led-matrix.h"
#include "bmi160.h"

asm (".global _printf_float");

/* this is global as this address is set up to be faulty */
int16_t gx;

void delay_ms(uint32_t period)
{
    uint64_t end = (period >> 1) + get_timer_value();
    while (get_timer_value() < end);
}

/* setup code */
void config_sensors(struct bmi160_dev *sensor)
{
    int8_t rslt = BMI160_OK;

    /* Select the Output data rate, range of accelerometer sensor */
    sensor->accel_cfg.odr = BMI160_ACCEL_ODR_1600HZ;
    sensor->accel_cfg.range = BMI160_ACCEL_RANGE_2G;
    sensor->accel_cfg.bw = BMI160_ACCEL_BW_NORMAL_AVG4;

    /* Sel->ct the power mode of accelerometer sensor */
    sensor->accel_cfg.power = BMI160_ACCEL_NORMAL_MODE;

    /* Sel->ct the Output data rate, range of Gyroscope sensor */
    sensor->gyro_cfg.odr = BMI160_GYRO_ODR_3200HZ;
    sensor->gyro_cfg.range = BMI160_GYRO_RANGE_2000_DPS;
    sensor->gyro_cfg.bw = BMI160_GYRO_BW_NORMAL_MODE;

    /* Select the power mode of Gyroscope sensor */
    sensor->gyro_cfg.power = BMI160_GYRO_NORMAL_MODE;

    /* Set the sensor configuration */
    rslt = bmi160_set_sens_conf(sensor);
}

void get_sensor_data(struct bmi160_dev *sensor, int16_t *x, int16_t *y, int16_t *z)
{
    int8_t rslt = BMI160_OK;
    struct bmi160_sensor_data accel;
    struct bmi160_sensor_data gyro;

    /* To read only Accel data */
    rslt = bmi160_get_sensor_data(BMI160_ACCEL_SEL, &accel, NULL, sensor);

    *x = accel.x;
    *y = accel.y;
    *z = accel.z;
}

/* rendering code for the led-matrix */

float cx = 15.0;
float cy = 4.0;
uint8_t dec_x;
uint8_t dec_y;

void render(void)
{
    dec_x = (uint8_t) cx;
    dec_y = (uint8_t) cy;

    clear();
    print_at(dec_x, dec_y);
}

void move_by_x(float dx)
{
    float nx = cx + dx;
    if ( nx > 0.0 && nx < 32.0) {
        cx = nx;
    }
}

void move_by_y(float dy)
{
    float ny = cy + dy;
    if ( ny > 0.0 && ny < 8.0) {
        cy = ny;
    }
}

float smooth_data(int16_t x)
{
    return (x / 17000.0);
}

/* fault activation code:
 * This is activated using the buttons 0 and 1 on the arty board
 * which tigger an interrupt each that we handle here
 */
uint8_t float_stuck = 0;
uint8_t random_flip = 0;
typedef void (*function_ptr_t) (void);
void no_interrupt_handler (void) {}
function_ptr_t g_ext_interrupt_handlers[PLIC_NUM_INTERRUPTS];
plic_instance_t g_plic;


void button_0_handler(void)
{
    float_stuck = 1 - float_stuck;
    printf("Float_stuck = %d\n", float_stuck);

    /* Custom fault injection registers:
     * We write the to be faulty memory address into CSR 0xbc0
     * and a mask to 0xbc1. Each set bit is then stuck-at 0
     */
    asm volatile ("csrw 0xbc0, %0" :: "r"(&gx));
    asm volatile ("csrw 0xbc1, %0" :: "r"(0xffff3000));

    /* CSR 0xbc4 holds the status flags for the fault injection registers.
     * Writing 0x1 activates it, writing 0x0 deactivates it.
     */
    if (float_stuck) {
        asm volatile ("csrw 0xbc4, %0" :: "r"(1));
    } else {
        asm volatile ("csrw 0xbc4, %0" :: "r"(0));
    }
    GPIO_REG(GPIO_RISE_IP) = (0x1 << BUTTON_0_OFFSET);
}

void button_1_handler(void)
{
    random_flip = 1 - random_flip;
    printf("random flip = %d\n", random_flip);

    /* 0xbc2 is the address register
     * 0xbc3 is a mask
     * On a read of address in 0xbc2 a random number is generated in hw and only
     * the bits indicated on the mask are returned
     */
    asm volatile ("csrw 0xbc2, %0" :: "r"(&dec_x));
    asm volatile ("csrw 0xbc3, %0" :: "r"(0x1f));
    if (random_flip) {
        asm volatile ("csrw 0xbc4, %0" :: "r"(2));
    } else {
        asm volatile ("csrw 0xbc4, %0" :: "r"(0));
    }
    GPIO_REG(GPIO_RISE_IP) = (0x1 << BUTTON_1_OFFSET);

}

void reset_demo(void)
{
    clear_csr(mie, MIP_MEIP);
    clear_csr(mie, MIP_MTIP);

    for (int ii = 0; ii < PLIC_NUM_INTERRUPTS; ii ++){
      g_ext_interrupt_handlers[ii] = no_interrupt_handler;
    }

    GPIO_REG(GPIO_OUTPUT_EN)  &= ~((0x1 << BUTTON_0_OFFSET) | (0x1 << BUTTON_1_OFFSET));
    GPIO_REG(GPIO_PULLUP_EN)  &= ~((0x1 << BUTTON_0_OFFSET) | (0x1 << BUTTON_1_OFFSET));
    GPIO_REG(GPIO_INPUT_EN)   |=  ((0x1 << BUTTON_0_OFFSET) | (0x1 << BUTTON_1_OFFSET));

    g_ext_interrupt_handlers[INT_DEVICE_BUTTON_0] = button_0_handler;
    g_ext_interrupt_handlers[INT_DEVICE_BUTTON_1] = button_1_handler;

    // Have to enable the interrupt both at the GPIO level,
    // and at the PLIC level.
    PLIC_enable_interrupt (&g_plic, INT_DEVICE_BUTTON_0);
    PLIC_enable_interrupt (&g_plic, INT_DEVICE_BUTTON_1);

    // Priority must be set > 0 to trigger the interrupt.
    PLIC_set_priority(&g_plic, INT_DEVICE_BUTTON_0, 1);
    PLIC_set_priority(&g_plic, INT_DEVICE_BUTTON_1, 1);

    GPIO_REG(GPIO_RISE_IE) |= (1 << BUTTON_0_OFFSET);
    GPIO_REG(GPIO_RISE_IE) |= (1 << BUTTON_1_OFFSET);

    // Enable the Machine-External bit in MIE
    set_csr(mie, MIP_MEIP);

    // Enable interrupts in general.
    set_csr(mstatus, MSTATUS_MIE);
    printf("Reset Demo\n");
}

void handle_m_ext_interrupt(){
  plic_source int_num  = PLIC_claim_interrupt(&g_plic);
  if ((int_num >=1 ) && (int_num < PLIC_NUM_INTERRUPTS)) {
    g_ext_interrupt_handlers[int_num]();
  }
  else {
    exit(1 + (uintptr_t) int_num);
  }
  PLIC_complete_interrupt(&g_plic, int_num);
}

int main()
{
    UART_init(115200, 0);

    spi_begin(PIN_10_OFFSET, 65000000);
    setup_matrix();

    struct bmi160_dev sensor;
    sensor.id = 0;
    sensor.interface = BMI160_SPI_INTF;
    sensor.read = &spi_read;
    sensor.write = &spi_write;
    sensor.delay_ms = &delay_ms;

    bmi160_init(&sensor);
    config_sensors(&sensor);
    PLIC_init(&g_plic, PLIC_CTRL_ADDR, PLIC_NUM_INTERRUPTS,
              PLIC_NUM_PRIORITIES);
    reset_demo();

    int16_t y, z;
    while (1) {
        get_sensor_data(&sensor, &gx, &y, &z);
        float fx = smooth_data(gx);
        float fy = smooth_data(y);

        if (fx > 0.03 || fx < -0.03) {
            move_by_y(-fx);
        }
        if (fy > 0.03 || fy < -0.03) {
            move_by_x(-fy);
        }
        render();
    }

    return 0;
}
