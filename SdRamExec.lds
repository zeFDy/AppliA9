OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start_SdRamPart)
SECTIONS
{
 . = 0x00000000;
 . = ALIGN(4);
 .text :
 {
 . = ALIGN(4);
  __image_copy_start = .;
  *(.vectorsSdRam)
  SdRamStart.o (.text*)
  . = 0x00000100;
  *(.mainSdRamPart)
  *(.text*)
  
 }
 . = ALIGN(4);
 .rodata : { *(SORT_BY_ALIGNMENT(SORT_BY_NAME(.rodata*))) }
 . = ALIGN(4);
 .data : {
  *(.data*)
 }
 . = ALIGN(4);
 __image_copy_end = .;
 .rel.dyn : {
  __rel_dyn_start = .;
  *(.rel*)
  __rel_dyn_end = .;
 }
 .end :
 {
  *(.__end)
 }
 _image_binary_end = .;
 .bss __rel_dyn_start (OVERLAY) : {
  __bss_start = .;
  *(.bss*)
   . = ALIGN(4);
  __bss_end = .;
 }
 __bss_size = __bss_end - __bss_start;
 .dynsym _image_binary_end : { *(.dynsym) }
 .dynbss : { *(.dynbss) }
 .dynstr : { *(.dynstr*) }
 .dynamic : { *(.dynamic*) }
 .hash : { *(.hash*) }
 .plt : { *(.plt*) }
 .interp : { *(.interp*) }
 .gnu : { *(.gnu*) }
 .ARM.exidx : { *(.ARM.exidx*) }
}

