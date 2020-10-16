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
#include "plic/plic_driver.h"
#include "common.h"
#include "UART_driver.h"
#include "watchdog.h"

asm (".global _printf_float");

#define NUM_Interrupts 59

/* this is global as this address is set up to be faulty */
void delay_ms(uint32_t period)
{
    uint64_t end = (period >> 1) + get_timer_value();
    while (get_timer_value() < end){asm("NOP");};
}

/* Create Watchdog Structs */
const struct wd_ints wd_interrupts[] = {
    {1},
    {53,54,55},
    {56},
    {57,58,59}
};

struct wd_settings wd_setting[] = {
                                    {(struct wd_unit*) 0x10000200, 16384, 1, &wd_interrupts[0], 0xD09F00D, 0x51F15E, 0, 0, "MockAON_timeout"},
                                    {(struct wd_unit*) 0x2000, 32500000, 3, &wd_interrupts[1], 0xD09F00D, 0x51F15E, 1, 0x48000000, "TLWDT_0_both"},
                                    {(struct wd_unit*) 0x4000, 16384, 1, &wd_interrupts[2], 0xD09F00D, 0x51F15E, 0, 0, "TLWDT_1_timeout"},
                                    {(struct wd_unit*) 0x6000, 16384, 3, &wd_interrupts[3], 0xD09F00D, 0x51F15E, 0, 0, "AXI4WDT_2_window"}
                                    };

// Register Map etc. https://sifive.cdn.prismic.io/sifive%2F500a69f8-af3a-4fd9-927f-10ca77077532_fe310-g000.pdf
// Architecture https://sifive.cdn.prismic.io/sifive%2Ffeb6f967-ff96-418f-9af4-a7f3b7fd1dfc_fe310-g000-ds.pdf
// Getting started Arty https://sifive.cdn.prismic.io/sifive%2Fed96de35-065f-474c-a432-9f6a364af9c8_sifive-e310-arty-gettingstarted-v1.0.6.pdf

enum eStates {uart_main,uart_unit,uart_mode,uart_scale,uart_counter,uart_compare,uart_pulsewidth,uart_mux,uart_zero,uart_rsten,uart_always,uart_awake,uart_interrupt,uart_invert, uart_select, uart_live};
enum eStates ConsoleState = uart_main;
enum eConsoleModes {console_stat, console_feed, console_invert, console_config, console_live};

plic_instance_t g_plic;

uint32_t char_to_uint(char *c, uint8_t len){
    uint32_t result = 0;
    uint32_t mult = 1;
    if (len >= 1){
        for(int16_t i = len-1; i>=0 ; i--){
            if(c[i] >= 0x30 && c[i] <= 0x39){   // Only numbers 0-9
                result += (c[i] - 0x30)*mult;
                mult *= 10;
            }
        }
    }
    return result;
}

void print_watchdog_times(uint32_t clock,uint8_t mode, uint8_t scale, uint64_t count, uint32_t compare0, uint32_t compare1, uint32_t pulsewidth){
    float t_period_scaled = 1.0/(float)(clock>>scale);
    float t_period = 1.0/(float)clock;
    if (count > compare0 || count > compare1){
        count = 0;
    }
    if(mode >0){
        printf("Time until Trigger is allowed: %f s \n", t_period_scaled*(float)(compare0-count));
        printf("Time in which Triggering is allowed: %f s \n", t_period_scaled*(float)(compare1-count));
    }else{
        printf("Time in which Triggering is allowed: %f s \n", t_period_scaled*(float)(compare0-count));
    }
    printf("Pulsetime: %f s \n", t_period*(float)pulsewidth);
}

void print_watchdog_config(uint8_t unit, uint8_t subunit){
    printf("Watchdog Configuration %s Unit: %i \n",wd_setting[unit].name,subunit);
    printf("    Inv: %i\n\
    Modus: %i\n\
    Scale: 2^%i\n\
    ZeroCmp: %i\n\
    RstEn: %i\n\
    AlwaysEn: %i\n\
    CoreAwakeEn: %i\n\
    Interrupt: %i\n\
    CounterHi: %lu\n\
    CounterLo: %lu\n\
    Scaled Counter: %lu\n\
    Compare0: %lu\n\
    Compare1: %lu\n\
    Pulsewidth: %lu\n\
    Mux: %lu\n\
    ",\
    wd_setting[unit].address->global.inv,\
    wd_setting[unit].address->unit[subunit].cfg.field.mode,\
    wd_setting[unit].address->unit[subunit].cfg.field.scale,\
    wd_setting[unit].address->unit[subunit].cfg.field.zerocmp,\
    wd_setting[unit].address->unit[subunit].cfg.field.outputen,\
    wd_setting[unit].address->unit[subunit].cfg.field.enalways,\
    wd_setting[unit].address->unit[subunit].cfg.field.encoreawake,\
    wd_setting[unit].address->unit[subunit].cfg.field.interrupt,\
    (uint32_t)((wd_setting[unit].address->unit[subunit].count & 0xFFFFFFFF00000000)>>32),\
    (uint32_t)(wd_setting[unit].address->unit[subunit].count & 0xFFFFFFFF),\
    (uint32_t)(wd_setting[unit].address->unit[subunit].s & 0xFFFFFFFF),/* Only lower 32 Bits */\  
    wd_setting[unit].address->unit[subunit].compare[0],\
    wd_setting[unit].address->unit[subunit].compare[1],\
    wd_setting[unit].address->unit[subunit].pulsewidth,\
    wd_setting[unit].address->unit[subunit].mux \
    );
    print_watchdog_times(wd_setting[unit].clock, wd_setting[unit].address->unit[subunit].cfg.field.mode, wd_setting[unit].address->unit[subunit].cfg.field.scale, wd_setting[unit].address->unit[subunit].count, wd_setting[unit].address->unit[subunit].compare[0], wd_setting[unit].address->unit[subunit].compare[1], wd_setting[unit].address->unit[subunit].pulsewidth);
}

void demo_WD_config(uint8_t selected_watchdog,uint8_t selected_subwatchdog){
    enum eStates State = uart_mode;
    struct wd_element temp_watchdog;
    char c;
    char buffer[20];
    uint32_t temp32;
    uint8_t temp8,ans;

    while(1){
        switch (State)
        {
            case uart_mode:
                printf("Enter Mode -> 0 = Timeout, 1 = Fenster\n");
                ans = UART_read_n(buffer, 20, '\r', 1);
                temp32 = char_to_uint(buffer,ans);
                if (temp32 >= 0 && temp32 <= 1){  // Check if Input is within [0,1] else display Error
                    temp_watchdog.cfg.field.mode = temp32;
                    State = uart_scale; 
                    printf("Mode Value: %i\n", temp_watchdog.cfg.field.mode);
                }else{
                    printf("Input %i out of Range. Type in a valid input otherwise use b or q to get back.\n", c);
                }
                break;
            case uart_scale:
                printf("Enter Scale Value -> Range 0-15 \n");
                ans = UART_read_n(buffer, 20, '\r', 1);
                temp32 = char_to_uint(buffer,ans);
                if (temp32 >= 0 && temp32 <= 15){  // Check if Input is within [0,15] else display Error
                    temp_watchdog.cfg.field.scale = temp32;
                    printf("Scale Value: %i\n", temp_watchdog.cfg.field.scale);
                    State = uart_counter; 
                }else{
                    printf("Input %i out of Range. Type in a valid input otherwise use b or q to get back.\n", temp32);
                }
                break;
            case uart_counter:
                printf("Enter Counter Value -> 32 Bit Range \n");
                ans = UART_read_n(buffer, 20, '\r', 1);
                temp32 = char_to_uint(buffer,ans);
                printf("Counter Value: %i\n",temp32);
                temp_watchdog.count = (uint64_t)temp32;
                printf("CounterHi Value: %lu\n",(uint32_t)((temp_watchdog.count&0xFFFFFFFF00000000)>>32) );
                printf("CounterLo Value: %lu\n",(uint32_t)(temp_watchdog.count&0xFFFFFFFF) );
                State = uart_compare; 
                temp8 = 0;
                break;
            case uart_compare:
                printf("Enter Compare %i Value -> 16 Bit Range \n", temp8);
                ans = UART_read_n(buffer, 20, '\r', 1);
                temp32 = char_to_uint(buffer,ans);
                if (temp32 <= 0xFFFF){  
                    temp_watchdog.compare[temp8] = temp32;
                    printf("Compare %i Value: %i\n",temp8, temp_watchdog.compare[temp8]);
                    if((temp_watchdog.cfg.field.mode >= 1 && temp8 >= 1 )||(temp_watchdog.cfg.field.mode == 0 && temp8 >= 0 )){
                        State = uart_pulsewidth; 
                    }else{
                        temp8 += 1;
                    }
                }else{
                    printf("Input %i out of Range. Type in a valid input otherwise use b or q to get back.\n", temp32);
                }
                break;
            case uart_pulsewidth:
                printf("Enter Pulsewidth Value -> 32 Bit Range \n");
                ans = UART_read_n(buffer, 20, '\r', 1);
                temp32 = char_to_uint(buffer,ans);
                if (temp32 <= 0xFFFFFFFF){  
                    temp_watchdog.pulsewidth = temp32;
                    printf("Pulsewidth Value: %i\n", temp_watchdog.pulsewidth);
                    print_watchdog_times(wd_setting[selected_watchdog].clock,temp_watchdog.cfg.field.mode, temp_watchdog.cfg.field.scale, temp_watchdog.count, temp_watchdog.compare[0], temp_watchdog.compare[1], temp_watchdog.pulsewidth );
                    State = uart_mux; 
                }else{
                    printf("Input %i out of Range. Type in a valid input otherwise use b or q to get back.\n", temp32);
                }
                break;
            case uart_mux:
                printf("Enter Mux Value -> 32 Bit Range \n");
                ans = UART_read_n(buffer, 20, '\r', 1);
                temp32 = char_to_uint(buffer,ans);  
                temp_watchdog.mux = temp32;
                printf("Mux Value: %lu\n", temp_watchdog.mux);
                State = uart_zero; 
                break;
            case uart_zero:
                printf("Enter ZeroCmp Value -> 0 = Timer hold it's value on reset, 1 = Timer will be 0 after reset\n");
                ans = UART_read_n(buffer, 20, '\r', 1);
                temp32 = char_to_uint(buffer,ans);
                if (temp32 >= 0 && temp32 <= 1){  // Check if Input is within [0,1] else display Error
                    temp_watchdog.cfg.field.zerocmp = temp32;
                    State = uart_rsten; 
                    printf("ZeroCmp Value: %i\n", temp_watchdog.cfg.field.zerocmp);
                }else{
                    printf("Input %i out of Range. Type in a valid input otherwise use b or q to get back.\n", c);
                }
                break;
            case uart_rsten:
                printf("Enter RstEN Value -> 0 = Resetoutput is Deactivated, 1 = Resetoutput is Activated\n");
                ans = UART_read_n(buffer, 20, '\r', 1);
                temp32 = char_to_uint(buffer,ans);
                if (temp32 >= 0 && temp32 <= 1){  // Check if Input is within [0,1] else display Error
                    temp_watchdog.cfg.field.outputen = temp32;
                    State = uart_always; 
                    printf("RstEN Value: %i\n", temp_watchdog.cfg.field.outputen);
                }else{
                    printf("Input %i out of Range. Type in a valid input otherwise use b or q to get back.\n", c);
                }
                break;
            case uart_always:
                printf("Enter CountAlways Value -> 0 = WD Counter is deactivated when CoreAwake 0, 1 = WD Counter is activated\n");
                ans = UART_read_n(buffer, 20, '\r', 1);
                temp32 = char_to_uint(buffer,ans);
                if (temp32 >= 0 && temp32 <= 1){  // Check if Input is within [0,1] else display Error
                    temp_watchdog.cfg.field.enalways = temp32;
                    State = uart_awake; 
                    printf("CountAlways Value: %i\n", temp_watchdog.cfg.field.enalways);
                }else{
                    printf("Input %i out of Range. Type in a valid input otherwise use b or q to get back.\n", c);
                }
                break;
            case uart_awake:
                printf("Enter CoreAwake Value -> 0 = WD Counter is deactivated when Always is 0, 1 = WD Counter is activated when Processor Core isn't sleeping \n");
                ans = UART_read_n(buffer, 20, '\r', 1);
                temp32 = char_to_uint(buffer,ans);
                if (temp32 >= 0 && temp32 <= 1){  // Check if Input is within [0,1] else display Error
                    temp_watchdog.cfg.field.encoreawake = temp32;
                    printf("CoreAwake Value: %i\n", temp_watchdog.cfg.field.encoreawake);
                    temp_watchdog.cfg.field.interrupt = 0;
                    if (selected_subwatchdog >= wd_setting[selected_watchdog].num_ints){
                        configWatchdog(&temp_watchdog, wd_setting[selected_watchdog].address, selected_subwatchdog, &wd_setting[selected_watchdog]);
                        print_watchdog_config(selected_watchdog, selected_subwatchdog);
                        return;
                    }else{
                        State = uart_interrupt; 
                    }
                }else{
                    printf("Input %i out of Range. Type in a valid input otherwise use b or q to get back.\n", c);
                }
                break;
            case uart_interrupt:
                printf("Select if Interrupt should be enabled for one Shot -> 0 = Interrupt disabled, 1 = Interrupt enabled \n");
                ans = UART_read_n(buffer, 20, '\r', 1);
                temp32 = char_to_uint(buffer,ans);
                if (temp32 >= 0 && temp32 <= 1){  // Check if Input is within [0,1] else display Error
                    configWatchdog(&temp_watchdog, wd_setting[selected_watchdog].address, selected_subwatchdog, &wd_setting[selected_watchdog]);
                    if (temp32){
                        GPIO_REG(GPIO_OUTPUT_VAL) ^= (1<<3);
                        PLIC_enable_interrupt (&g_plic, wd_setting[selected_watchdog].ints->channels[selected_subwatchdog]);
                        PLIC_set_priority(&g_plic, wd_setting[selected_watchdog].ints->channels[selected_subwatchdog], 1);
                        printf("Interrupt Channel %i enabled \n", wd_setting[selected_watchdog].ints->channels[selected_subwatchdog]);
                    }else{
                        printf("Interrupt disabled \n");
                    }
                    print_watchdog_config(selected_watchdog, selected_subwatchdog);
                    return;
                }else{
                    printf("Input %i out of Range. Type in a valid input otherwise use b or q to get back.\n", c);
                }
                break;
            default:
                break;
        }
        if(c == 'q' || buffer[0] == 'q'){
            return;
        }
        if(c == 'b' || buffer[0] == 'b'){
            c = 0;
            buffer[0] = 0;
            if (State == uart_mode){
                return;
            }else{
                State--;
            }
            return;
        }
    }
}

void Console_Task(){
    struct wd_element temp_watchdog;
    uint8_t selected_watchdog = 0;
    uint8_t selected_subwatchdog = 0;
    char c;
    char buffer[20];
    uint32_t temp32;
    uint8_t temp8,ans;
    enum eConsoleModes cMode = console_stat;

    uint8_t elements = (sizeof(wd_setting)/sizeof(wd_setting[0]));
    uint8_t i = 0;
    while(1){
        
        switch (ConsoleState)
        {
            case uart_main:
                printf("\n\
                --------------------------------------\n\
                --------------------------------------\n\
                ------- Watchdog Demo Main Menue -----\n\
                --------------------------------------\n\
                --------------------------------------\n\
                    \n\
                    C - Config \n\
                    S - Status \n\
                    I - Invert Output \n\
                    F - Feed Watchdog\n\
                    L - Live Status\n\
                    \n\
                    b - back to previous Menue\n\
                    q - back to Main Menue\n\
                ");
                ans = UART_get_char(&c,1);
                if(ans == 0){
                    switch(c){
                        case 'C':
                            cMode = console_config;
                            ConsoleState = uart_select;
                            break;
                        case 'S':
                            cMode = console_stat;
                            ConsoleState = uart_select; 
                            break;
                        case 'I':
                            cMode = console_invert;
                            ConsoleState = uart_select;
                            break;
                        case 'F':
                            cMode = console_feed;
                            ConsoleState = uart_select;
                            break;
                        case 'L':
                            cMode = console_live;
                            ConsoleState = uart_select;
                            break;
                        case 'b':
                            break;
                        case 'q':
                            break;
                    }
                }
                break;
            
            case uart_select:
                
                printf("\rSelect WD: \n");
                for(i = 0; i<elements;i++){
                    printf("%i -> %s \n",i,wd_setting[i].name);
                }
                ans = UART_read_n(buffer, 20, '\r', 1);
                temp32 = char_to_uint(buffer,ans);
                if (temp32 >= 0 && temp32 <= elements){  
                    selected_watchdog = temp32; 
                     printf("Watchdog %s selected, Interrupt Channels=(", wd_setting[selected_watchdog].name);
                     for(i = 0; i < wd_setting[selected_watchdog].num_ints; i++){
                         if(i != 0){
                             printf(",");
                         }
                         printf("%i", wd_setting[selected_watchdog].ints->channels[i]);
                     }
                     printf(") \n");
                     if (cMode != console_invert){
                         ConsoleState = uart_unit; 
                     }else{
                         ConsoleState = uart_invert;
                     }
                }else{
                    printf("Input %i out of Range. Type in a valid input otherwise use b or q to get back.\n", temp32);
                }
                break;
            case uart_unit:
                printf("Enter WD Subunit -> Range 0-32\n");
                ans = UART_read_n(buffer, 20, '\r', 1);
                temp32 = char_to_uint(buffer,ans);
                if (temp32 >= 0 && temp32 <= 32){  // Check if Input is within [0,32] else display Error
                    selected_subwatchdog = temp32;
                    printf("Unit %i selected\n", selected_subwatchdog);
                    if(cMode != console_feed && cMode != console_live){
                        print_watchdog_config(selected_watchdog, selected_subwatchdog);
                        if(cMode == console_config){
                            demo_WD_config(selected_watchdog,selected_subwatchdog);
                        }
                    }else if(cMode == console_live){
                        ConsoleState = uart_live; 
                        printf("Press 'F' for Feeding Watchdog\n");
                        break;
                    }else{
                        unlock(wd_setting[selected_watchdog].address,&wd_setting[selected_watchdog]);
                        wd_setting[selected_watchdog].address->unit[selected_subwatchdog].feed = wd_setting[selected_watchdog].food;
                        printf("WD got food.");
                    }                    
                    ConsoleState = uart_main; 
                    break;
                }else{
                    printf("Input %i out of Range. Type in a valid input otherwise use b or q to get back.\n", temp32);
                }
                break;

            case uart_live:
                printf("\rScaled Counter Value: %6lu",(uint32_t)(wd_setting[selected_watchdog].address->unit[selected_subwatchdog].s & 0xFFFFFFFF));
                ans = UART_read_n(buffer, 1, '\r', 0);
                if (ans > 0){
                    if (buffer[0] == 'F'){
                        unlock(wd_setting[selected_watchdog].address,&wd_setting[selected_watchdog]);
                        wd_setting[selected_watchdog].address->unit[selected_subwatchdog].feed = wd_setting[selected_watchdog].food;
                        printf("\n  WD got food.\n");
                    }else if(buffer[0] == '\r'){
                        printf("\n");
                    }
                }
                delay_ms(100);
                break;
            case uart_invert:
                printf("Enter Channel to Invert -> Range 0-32\n");
                ans = UART_read_n(buffer, 20, '\r', 1);
                temp32 = char_to_uint(buffer,ans);
                if (temp32 >= 0 && temp32 <= 32){  // Check if Input is within [0,32] else display Error
                    printf("No Inversion -> 0 Inversion -> 1 \n");
                    ans = UART_read_n(buffer, 10, '\r', 1);
                    temp8 = char_to_uint(buffer,ans);
                    if (temp8 == 0 ){
                        printf("Channel %i won't be inverted\n", temp32);
                        unlock(wd_setting[selected_watchdog].address, &wd_setting[selected_watchdog]);
                        nInvOutput(wd_setting[selected_watchdog].address, temp32);
                        ConsoleState = uart_main; 
                    }else if(temp8 == 1){  
                        printf("Channel %i will be inverted\n", temp32);
                        unlock(wd_setting[selected_watchdog].address, &wd_setting[selected_watchdog]);
                        InvOutput(wd_setting[selected_watchdog].address, temp32);
                        ConsoleState = uart_main; 
                    }else{
                        printf("Input %i out of Range. Type in a valid input otherwise use b or q to get back.\n", temp8);
                    }
                }else{
                    printf("Input %i out of Range. Type in a valid input otherwise use b or q to get back.\n", temp32);
                }
                break;
            
            default:
                ConsoleState = uart_main; 
                break;
        }
        if(c == 'q' || buffer[0] == 'q' || c == 'b' || buffer[0] == 'b'){
            c = 0;
            buffer[0] = 0;
            ConsoleState = uart_main;
        }   
    }
}

void handle_m_ext_interrupt(){
    uint8_t x,i;
    plic_source int_num  = PLIC_claim_interrupt(&g_plic);
    // Find Watchdog with corresponding Interrupt channel
    for(i = 0;i< sizeof(wd_setting)/sizeof(wd_setting[0]);i++){
        for(x = 0; x < wd_setting[i].num_ints; x++ ){
            if(wd_setting[i].ints->channels[x] == int_num){
                break;
            }
        }
        if(x < wd_setting[i].num_ints){
            break;
        }
    }
    // Disable Watchdog and reset Interrupt
    unlock(wd_setting[i].address, &wd_setting[i]);
    writeReg(wd_reg_ctrl_ip_0,0, &wd_setting[i].address->unit[x] );
    unlock(wd_setting[i].address, &wd_setting[i]);
    writeReg(wd_reg_ctrl_running,0, &wd_setting[i].address->unit[x] );
    unlock(wd_setting[i].address, &wd_setting[i]);
    writeReg(wd_reg_ctrl_countAlways,0, &wd_setting[i].address->unit[x] );

    GPIO_REG(GPIO_OUTPUT_VAL) ^= (1<<3);

    PLIC_complete_interrupt(&g_plic, int_num);
    PLIC_disable_interrupt (&g_plic, int_num);
}

extern void trap_entry();
void init_plic(void)
{
    printf("mtvec: 0x%X \n", read_csr(mtvec));
    write_csr(mtvec, &trap_entry);
    
    PLIC_init(&g_plic, PLIC_CTRL_ADDR, NUM_Interrupts,
              PLIC_NUM_PRIORITIES);

    clear_csr(mie, MIP_MEIP);
    clear_csr(mie, MIP_MTIP);

    // Enable the Machine-External bit in MIE
    set_csr(mie, MIP_MEIP);
    // Enable interrupts in general.
    set_csr(mstatus, MSTATUS_MIE);
    //write_csr(mstatus, 0x1808);
    printf("PLIC Configured mstatus: 0x%X \n", read_csr(mstatus));
}

int main()
{
    
    UART_init(250000, 0);

    printf("core freq at %d Hz\n", get_cpu_freq());
    printf("Elements: %i \n", sizeof(wd_setting)/sizeof(wd_setting[0]));

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

    init_plic();
    Console_Task();
    return 0;
}

