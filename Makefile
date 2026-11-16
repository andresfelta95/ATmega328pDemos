# Build the ATmega328P demos with avr-gcc (no Microchip Studio needed).
#
#   make                    build every demo and compile every driver in lib/
#   make tof                build one demo (blink, neopix, tof)
#   make flash-tof          build and upload a demo with avrdude
#   make size               show flash / RAM use of every demo
#   make clean              delete the build/ folder
#
# Settings can be overridden on the command line, for example:
#   make flash-blink PORT=COM3
#   make flash-neopix PROGRAMMER=usbasp PORT=usb

MCU        ?= atmega328p
F_CPU      ?= 16000000UL

# avrdude settings (defaults suit an Arduino Uno; old-bootloader Nano: BAUD=57600)
AVRDUDE_MCU ?= m328p
PROGRAMMER ?= arduino
PORT       ?= /dev/ttyACM0
BAUD       ?= 115200

CC      := avr-gcc
OBJCOPY := avr-objcopy
SIZE    := avr-size
AVRDUDE := avrdude

BUILD := build

CFLAGS  := -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Os -std=gnu99 -Wall \
           -ffunction-sections -fdata-sections -MMD -MP -Ilib/inc
LDFLAGS := -mmcu=$(MCU) -Wl,--gc-sections

# ---------------------------------------------------------------------------
# Demos: <name>_SRCS, plus optional <name>_INCS and <name>_LIBS
# ---------------------------------------------------------------------------
DEMOS := blink neopix tof

blink_SRCS  := Demos/GccDemo01/GccDemo01/main.c

neopix_SRCS := Demos/GCC_NeoPix/GCC_NeoPix/main.c \
               lib/src/NeoPix.c

TOF_DIR     := Demos/GccI2CV01/GccI2CV01
tof_SRCS    := $(TOF_DIR)/main.c \
               lib/src/I2C328P.c \
               lib/src/SSD1306.c \
               $(wildcard $(TOF_DIR)/API/core/*.c) \
               $(wildcard $(TOF_DIR)/API/platform/*.c)
tof_INCS    := -I$(TOF_DIR)/API/core -I$(TOF_DIR)/API/platform
tof_LIBS    := -lm

# Every driver in lib/ is also compiled on its own, so drivers
# that no demo uses yet (sci, timer, atd, LM75A) still get checked
LIB_SRCS := $(wildcard lib/src/*.c)
LIB_OBJS := $(LIB_SRCS:%.c=$(BUILD)/lib-check/%.o)

# ---------------------------------------------------------------------------

.PHONY: all lib size clean help $(DEMOS) $(addprefix flash-,$(DEMOS))

all: $(DEMOS) lib

lib: $(LIB_OBJS)

$(BUILD)/lib-check/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Rules generated for each demo: build/<demo>/<demo>.elf and .hex, flash-<demo>
define DEMO_RULES
$(1)_OBJS := $$($(1)_SRCS:%.c=$(BUILD)/$(1)/%.o)

$(1): $(BUILD)/$(1)/$(1).hex

$(BUILD)/$(1)/%.o: %.c
	@mkdir -p $$(dir $$@)
	$(CC) $(CFLAGS) $$($(1)_INCS) -c $$< -o $$@

$(BUILD)/$(1)/$(1).elf: $$($(1)_OBJS)
	$(CC) $(LDFLAGS) $$^ $$($(1)_LIBS) -o $$@

flash-$(1): $(BUILD)/$(1)/$(1).hex
	$(AVRDUDE) -c $(PROGRAMMER) -p $(AVRDUDE_MCU) -P $(PORT) -b $(BAUD) -U flash:w:$$<:i

-include $$($(1)_OBJS:.o=.d)
endef

$(foreach d,$(DEMOS),$(eval $(call DEMO_RULES,$(d))))

%.hex: %.elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

size: $(foreach d,$(DEMOS),$(BUILD)/$(d)/$(d).elf)
	@for f in $^; do echo "== $$f"; $(SIZE) -C --mcu=$(MCU) $$f | grep -E 'Program|Data'; done

clean:
	rm -rf $(BUILD)

help:
	@sed -n '2,12p' Makefile | sed 's/^# \{0,1\}//'

-include $(LIB_OBJS:.o=.d)
