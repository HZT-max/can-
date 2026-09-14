ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 64K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 16K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);
_stack_size = 2K;
_stack_bottom = _estack - _stack_size;

SECTIONS
{
    .isr_vector :
    {
        KEEP(*(.isr_vector))
    } > FLASH

    .text :
    {
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    .bss (NOLOAD) :
    {
        . = ALIGN(8);
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(8);
        _ebss = .;
    } > RAM

    ASSERT((_sidata & 3) == 0, ".data Flash load address must be 4-byte aligned")
    ASSERT(_ebss <= _stack_bottom, "RAM overflow: static data collides with reserved stack")

    /DISCARD/ :
    {
        *(.comment*)
        *(.ARM.attributes*)
        *(.ARM.exidx*)
        *(.ARM.extab*)
    }
}
