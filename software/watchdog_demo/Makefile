TARGET = watchdog_demo
C_SRCS += demo.c common.c UART_driver.c watchdog.c
CFLAGS += -O2 -fno-builtin-printf -DUSE_PLIC 

C_SRCS += $(BSP_BASE)/drivers/plic/plic_driver.c
BSP_BASE = ../../bsp
include $(BSP_BASE)/env/common.mk
