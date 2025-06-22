/* Linker script for GCC build */
MEMORY
{
  INFOMEM (r)  : ORIGIN = 0x00000000, LENGTH = 0x100
  RAM     (rwx) : ORIGIN = 0x20000004, LENGTH = 0x3FFFC
}

ENTRY(Init)

SECTIONS
{
  .text :
  {
    *(.text*)
    *(.rodata*)
  } > RAM

  .data :
  {
    *(.data*)
  } > RAM

  .bss :
  {
    __bss_start__ = .;
    *(.bss*)
    *(COMMON)
    __bss_end__ = .;
  } > RAM

  .info :
  {
    *(.info*)
  } > INFOMEM
}
