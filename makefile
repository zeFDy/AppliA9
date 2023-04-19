TARGET = myBoot.o

#CROSS_COMPILE = arm-linux-gnueabihf-
CROSS_COMPILE = "C:/Program Files (x86)/GNU Arm Embedded Toolchain/10 2021.07/bin/arm-none-eabi-"
CC = $(CROSS_COMPILE)gcc
AS = $(CROSS_COMPILE)as
LD = $(CROSS_COMPILE)ld.bfd 
CP = $(CROSS_COMPILE)objcopy 
OD = $(CROSS_COMPILE)objdump
ARCH = arm

CCOPT = -mthumb -mthumb-interwork -mabi=aapcs-linux  -mno-unaligned-access -ffunction-sections -fdata-sections -fno-common -ffixed-r9  -msoft-float -march=armv7-a
LDSCRIPT := myBoot.lds

OPTIONS = -Os -Wall -Wstrict-prototypes -Wno-format-security -fno-builtin -ffreestanding -mthumb -mthumb-interwork -mabi=aapcs-linux -mword-relocations  -fno-pic  -mno-unaligned-access -mno-unaligned-access -ffunction-sections -fdata-sections -fno-common -ffixed-r9 -msoft-float -march=armv7-a -P -c 



hwlibs/src/alt_clock_manager.o:	hwlibs/src/alt_clock_manager.c
							$(CC) $(OPTIONS) $^ -o $@
							$(OD) -d $@ >$@.lst

hwlibs/src/alt_globaltmr.o:	hwlibs/src/alt_globaltmr.c
							$(CC) $(OPTIONS) $^ -o $@
							$(OD) -d $@ >$@.lst
							
hwlibs/src/alt_timers.o:	hwlibs/src/alt_timers.c
							$(CC) $(OPTIONS) $^ -o $@
							$(OD) -d $@ >$@.lst

hwlibs/src/alt_interrupt.o:	hwlibs/src/alt_interrupt.c
							$(CC) $(OPTIONS) $^ -o $@
							$(OD) -d $@ >$@.lst

hwlibs/src/alt_watchdog.o:	hwlibs/src/alt_watchdog.c
							$(CC) $(OPTIONS) $^ -o $@
							$(OD) -d $@ >$@.lst


							
SdRamStart.o:	SdRamStart.S
				$(CC) -x assembler-with-cpp SdRamStart.S -o SdRamStart.o -c
				$(OD) -d SdRamStart.o >SdRamStart.lst
				
SdRamMain.o:	SdRamMain.c
				$(CC) $(OPTIONS) $^ -o $@
				$(OD) -d $@ >$@.lst

debugConsole.o:	debugConsole.c
				$(CC) $(OPTIONS) $^ -o $@
				$(OD) -d $@ >$@.lst

SetUpTimers.o:	SetUpTimers.c
				$(CC) $(OPTIONS) $^ -o $@
				$(OD) -d $@ >$@.lst

SdRamExec.o:	SdRamStart.o 							\
				debugConsole.o							\
				SetUpTimers.o							\
				hwlibs/src/alt_clock_manager.o 			\
				hwlibs/src/alt_globaltmr.o 				\
				hwlibs/src/alt_interrupt.o				\
				hwlibs/src/alt_timers.o 				\
				hwlibs/src/alt_watchdog.o 				\
				SdRamMain.o
				$(LD) -T SdRamExec.lds SdRamStart.o 	\
				SdRamMain.o 							\
				debugConsole.o							\
				SetUpTimers.o							\
				hwlibs/src/alt_clock_manager.o 			\
				hwlibs/src/alt_globaltmr.o 				\
				hwlibs/src/alt_interrupt.o				\
				hwlibs/src/alt_timers.o 				\
				hwlibs/src/alt_watchdog.o 				\
				lib.a 									\
				-o SdRamExec.o
				$(CP) -O binary SdRamExec.o SdRamExec.bin
				$(CP) --srec-forceS3 -O srec SdRamExec.o SdRamExec.txt	
				$(OD) -d SdRamExec.o >SdRamExec.lst
				$(OD) -x SdRamExec.o >SdRamExec.map
				./bin2raw SdRamExec.bin SdRamExec.raw


