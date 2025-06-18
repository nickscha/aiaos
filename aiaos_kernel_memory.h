#ifndef AIAOS_KERNEL_MEMORY_H
#define AIAOS_KERNEL_MEMORY_H

/* aiaos_linker.ld variables for memory addresses */
extern char _stack_top; /* Stack top address as specified in aiaios_linker.ld*/
extern unsigned long MEM_TOTAL;

#define AIAOS_KERNEL_MEMORY_OFFSET ((unsigned long)&_stack_top) /* Stack Offset. See aiaos_linker.ld */

static void *aiaos_kernel_memory = 0;
static unsigned long aiaos_kernel_memory_size = 0;

void aiaos_kernel_memory_initialize(void)
{
    aiaos_kernel_memory = (void *)(unsigned long)(AIAOS_KERNEL_MEMORY_OFFSET);
    aiaos_kernel_memory_size = (unsigned long)(MEM_TOTAL - AIAOS_KERNEL_MEMORY_OFFSET);
}

#define PCIE_MMIO_RESERVED_START 0xE0000000UL
#define PCIE_MMIO_RESERVED_END 0xFFFFFFFFUL

void aiaos_kernel_memory_zero(void *ptr, unsigned long size)
{
    unsigned long start = (unsigned long)ptr;
    unsigned long end = start + size;
    unsigned long current = start;

    while (current < end)
    {
        unsigned long chunk_start = current;
        unsigned long chunk_end = end;
        unsigned long chunk_size;
        unsigned long num_qwords;
        unsigned long remainder;
        void *chunk_ptr;

        if (chunk_start < PCIE_MMIO_RESERVED_END && chunk_end > PCIE_MMIO_RESERVED_START)
        {
            if (chunk_start < PCIE_MMIO_RESERVED_START)
            {
                chunk_end = PCIE_MMIO_RESERVED_START;
            }
            else
            {
                break;
            }
        }

        chunk_size = chunk_end - chunk_start;
        num_qwords = chunk_size / 8;
        remainder = chunk_size % 8;
        chunk_ptr = (void *)chunk_start;

        __asm __volatile(
            "rep stosq"
            : "+D"(chunk_ptr), "+c"(num_qwords)
            : "a"(0)
            : "memory");

        if (remainder > 0)
        {
            unsigned char *p8 = (unsigned char *)chunk_ptr;
            while (remainder--)
            {
                *p8++ = 0;
            }
        }

        current = chunk_end;

        if (current == PCIE_MMIO_RESERVED_START)
        {
            current = PCIE_MMIO_RESERVED_END + 1;
        }
    }
}

#endif /* AIAOS_KERNEL_MEMORY_H */
