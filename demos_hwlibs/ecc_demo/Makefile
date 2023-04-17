#
# Copyright Altera 2015
# All Rights Reserved.
#
ALT_DEVICE_FAMILY ?= soc_cv_av
SOCEDS_ROOT ?= $(SOCEDS_DEST_ROOT)
HWLIBS_ROOT = $(SOCEDS_ROOT)/ip/altera/hps/altera_hps/hwlib

HWLIBS_SRC  := alt_interrupt.c alt_ecc.c alt_qspi.c alt_globaltmr.c alt_clock_manager.c alt_sdmmc.c alt_dma.c alt_dma_program.c alt_cache.c alt_mmu.c memcpy_s.c safe_mem_constraint.c mem_primitives_lib.c ignore_handler_s.c
EXAMPLE_SRC := ecc_demo.c ecc_demo_ocram.c ecc_demo_qspi.c ecc_demo_sdmmc.c ecc_demo_dma.c alt_pt.c ecc_demo_l2.c
C_SRC       := $(EXAMPLE_SRC) $(HWLIBS_SRC)

LINKER_SCRIPT := cycloneV-dk-ram-hosted.ld

MULTILIBFLAGS := -mcpu=cortex-a9 -mfloat-abi=softfp -mfpu=neon
CFLAGS  := -g -O0 -Wall -Werror -std=c99 $(MULTILIBFLAGS) -I$(HWLIBS_ROOT)/include -I$(HWLIBS_ROOT)/include/$(ALT_DEVICE_FAMILY) -D$(ALT_DEVICE_FAMILY) -I$(HWLIBS_ROOT)/src/safeclib/ -I$(HWLIBS_ROOT)/include/safeclib/
LDFLAGS := -T$(LINKER_SCRIPT) $(MULTILIBFLAGS)

CROSS_COMPILE := arm-altera-eabi-
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)g++
NM := $(CROSS_COMPILE)nm
OD := $(CROSS_COMPILE)objdump
OC := $(CROSS_COMPILE)objcopy

RM := rm -rf
CP := cp -f

ELF ?= $(basename $(firstword $(C_SRC))).axf
SPL := u-boot-spl.axf 
OBJ := $(patsubst %.c,%.o,$(C_SRC))

.PHONY: all
all: $(ELF) $(SPL)

.PHONY: clean
clean:
	$(RM) $(ELF) $(OBJ) *.objdump *.map $(HWLIBS_SRC) $(SPL)
	
define SET_HWLIBS_DEPENDENCIES
$(1): $(2)
	$(CP) $(2) $(1)
endef

ALL_HWLIBS_SRC = $(wildcard $(HWLIBS_ROOT)/src/hwmgr/*.c) $(wildcard $(HWLIBS_ROOT)/src/hwmgr/$(ALT_DEVICE_FAMILY)/*.c)  $(wildcard $(HWLIBS_ROOT)/src/safeclib/*.c)

$(foreach file,$(ALL_HWLIBS_SRC),$(eval $(call SET_HWLIBS_DEPENDENCIES,$(notdir $(file)),$(file))))

alt_ecc.c: $(HWLIBS_ROOT)/src/hwmgr/$(ALT_DEVICE_FAMILY)/alt_ecc.c alt_ecc_scratch_ram_get.c
	echo "Adding function to alt_ecc.c ..."
	$(CP) $< $@
	chmod +w $@
	cat alt_ecc_scratch_ram_get.c >> $@
	
$(OBJ): %.o: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

$(ELF): $(OBJ)
	$(LD) $(LDFLAGS) $(OBJ) -o $@
	$(OD) -d $@ > $@.objdump
	$(NM) $@ > $@.map

$(SPL): $(SOCEDS_ROOT)/examples/hardware/cv_soc_devkit_ghrd/software/preloader/uboot-socfpga/spl/u-boot-spl
	$(CP) $< $@
	$(OD) -d $@ > $@.objdump
