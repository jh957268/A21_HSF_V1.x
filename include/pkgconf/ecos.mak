ECOS_GLOBAL_CFLAGS = -Wall -Wpointer-arith -Wstrict-prototypes -Wundef -Wno-write-strings -EL -mips32 -msoft-float -g -gstabs -O2 -ffunction-sections -fdata-sections -fno-rtti -fno-exceptions -G0 -DCONFIG_MT7628_ASIC
ECOS_GLOBAL_LDFLAGS = -EL -mips32 -msoft-float -g -nostdlib -Wl,--gc-sections -Wl,-static
ECOS_COMMAND_PREFIX = mipsisa32-elf-
