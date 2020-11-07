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
#include "platform.h"
#include "encoding.h"
#include "common.h"
#include "UART_driver.h"

asm (".global _printf_float");

uint8_t* mem = (uint8_t*) 0x8000;


int main()
{
    UART_init(250000, 0);

    printf("core freq at %d Hz\n", get_cpu_freq());

    // Set IO Function to WD Periph 1 and 2
    GPIO_REG(GPIO_IOF_SEL)    &= ~(1<<19);
    GPIO_REG(GPIO_IOF_EN)     |= (1<<19);
    GPIO_REG(GPIO_IOF_SEL)    &= ~(1<<21);
    GPIO_REG(GPIO_IOF_EN)     |= (1<<21);
    GPIO_REG(GPIO_IOF_SEL)    &= ~(1<<22);
    GPIO_REG(GPIO_IOF_EN)     |= (1<<22);

    // LD1 blue
    GPIO_REG(GPIO_IOF_EN)     &= ~(1<<3);
    GPIO_REG(GPIO_INPUT_EN)   &= ~(1<<3);
    GPIO_REG(GPIO_OUTPUT_EN)  |= (1<<3);
    for(uint32_t i = 0; i<63; i++){
        mem[i] = 63-i;
        if(mem[i] == 63-i){
            printf("Correct Data at %i Data= %i \n", i, mem[i]);
        }else{
            printf("Wrong Data at %i Data= %i \n", i, mem[i]);
        }         
    }
    while(1){
        ;
    }
    return 0;
}

/* this is global as this address is set up to be faulty */
void delay_ms(uint32_t period)
{
    uint64_t end = (period >> 1) + get_timer_value();
    while (get_timer_value() < end){asm("NOP");};
}
