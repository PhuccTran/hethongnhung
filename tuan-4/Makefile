PROJECT := freertos_std
BUILD := build
TARGET := $(BUILD)/$(PROJECT)

PREFIX ?= arm-none-eabi-
CC := $(PREFIX)gcc
AS := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
SIZE := $(PREFIX)size
STM32FLASH ?= stm32flash
PORT ?= /dev/ttyUSB0
BAUD ?= 115200

SPL := vendor/STM32F10x_StdPeriph_Lib
FREERTOS := vendor/FreeRTOS-Kernel
DEVICE := $(SPL)/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x

CFLAGS := -mcpu=cortex-m3 -mthumb -mfloat-abi=soft
CFLAGS += -std=gnu11 -Wall -Wextra -Werror -Os -ffunction-sections -fdata-sections
CFLAGS += -DUSE_STDPERIPH_DRIVER -DSTM32F10X_MD -DHSE_VALUE=8000000U
CFLAGS += -Iapp -I$(SPL)/Libraries/CMSIS/CM3/CoreSupport
CFLAGS += -I$(DEVICE) -I$(SPL)/Libraries/STM32F10x_StdPeriph_Driver/inc
CFLAGS += -I$(FREERTOS)/include -I$(FREERTOS)/portable/GCC/ARM_CM3

LDFLAGS := -mcpu=cortex-m3 -mthumb -mfloat-abi=soft
LDFLAGS += -Tlinker/STM32F103C8Tx_FLASH.ld -Wl,--gc-sections -Wl,-Map=$(TARGET).map
LDFLAGS += --specs=nosys.specs --specs=nano.specs

SPL_SRC := $(wildcard $(SPL)/Libraries/STM32F10x_StdPeriph_Driver/src/*.c)
FREERTOS_SRC := $(FREERTOS)/tasks.c $(FREERTOS)/queue.c $(FREERTOS)/list.c
FREERTOS_SRC += $(FREERTOS)/timers.c $(FREERTOS)/event_groups.c $(FREERTOS)/stream_buffer.c
FREERTOS_SRC += $(FREERTOS)/portable/GCC/ARM_CM3/port.c $(FREERTOS)/portable/MemMang/heap_4.c
APP_SRC := app/main.c app/debug.c app/gpio_toggle_task.c $(DEVICE)/system_stm32f10x.c
ASM_SRC := $(DEVICE)/startup/gcc_ride7/startup_stm32f10x_md.s
SRC := $(APP_SRC) $(SPL_SRC) $(FREERTOS_SRC)
OBJ := $(patsubst %.c,$(BUILD)/%.o,$(SRC)) $(patsubst %.s,$(BUILD)/%.o,$(ASM_SRC))

.PHONY: all clean flash size

all: $(TARGET).hex $(TARGET).bin size

$(TARGET).elf: $(OBJ) linker/STM32F103C8Tx_FLASH.ld
	@mkdir -p $(dir $@)
	$(CC) $(OBJ) $(LDFLAGS) -o $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.s
	@mkdir -p $(dir $@)
	$(AS) $(CFLAGS) -x assembler-with-cpp -c $< -o $@

size: $(TARGET).elf
	$(SIZE) $<

flash: $(TARGET).bin
	$(STM32FLASH) -b $(BAUD) -w $< -v -g 0x08000000 $(PORT)

clean:
	rm -rf $(BUILD)
