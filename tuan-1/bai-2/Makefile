TARGET := bai2
BUILD := build

CC := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
SIZE := arm-none-eabi-size

CFLAGS := -mcpu=cortex-m3 -mthumb -O2 -ffreestanding -fno-builtin \
           -fno-stack-protector -Wall -Wextra -Werror
LDFLAGS := -T linker.ld -nostdlib -Wl,--gc-sections -Wl,-Map=$(BUILD)/$(TARGET).map

OBJECTS := $(BUILD)/startup.o $(BUILD)/main.o

.PHONY: all clean flash flash-openocd flash-stlink erase info

OPENOCD := openocd
OPENOCD_INTERFACE := interface/stlink.cfg
OPENOCD_TARGET := target/stm32f1x.cfg

all: $(BUILD)/$(TARGET).bin

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/startup.o: startup.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/main.o: main.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/$(TARGET).elf: $(OBJECTS) linker.ld
	$(CC) $(CFLAGS) $(OBJECTS) $(LDFLAGS) -o $@
	$(SIZE) $@

$(BUILD)/$(TARGET).bin: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

flash: flash-openocd

flash-openocd: $(BUILD)/$(TARGET).bin
	$(OPENOCD) -f $(OPENOCD_INTERFACE) -f $(OPENOCD_TARGET) \
		-c "program $< 0x08000000 verify reset exit"

flash-stlink: $(BUILD)/$(TARGET).bin
	st-flash --reset write $< 0x08000000

erase:
	$(OPENOCD) -f $(OPENOCD_INTERFACE) -f $(OPENOCD_TARGET) \
		-c "init; reset halt; stm32f1x mass_erase 0; shutdown"

info:
	$(OPENOCD) -f $(OPENOCD_INTERFACE) -f $(OPENOCD_TARGET) -c "init; reset halt; shutdown"

clean:
	rm -rf $(BUILD)