CC=arm-none-eabi-gcc
CFLAGS=-mcpu=cortex-m33 -mthumb -O2 -ffunction-sections -fdata-sections \
       -IIncludes -IIncludes/Loader -IIncludes/Library -IIncludes/CMSIS
LDFLAGS=-Tgcc/loader.ld -nostartfiles -specs=nosys.specs -Wl,--gc-sections

SOURCES=$(wildcard Sources/Loader/*.c) $(wildcard Sources/Library/*.c)
OBJECTS=$(SOURCES:.c=.o)

all: loader.elf

loader.elf: $(OBJECTS) gcc/loader.ld
	$(CC) $(CFLAGS) $(OBJECTS) $(LDFLAGS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -f $(OBJECTS) loader.elf

.PHONY: all clean
