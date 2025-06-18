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

void aiaos_kernel_memory_zero(void *ptr, unsigned long size)
{
    unsigned long num_qwords = size / 8;
    unsigned long remainder = size % 8;

    __asm__ __volatile__(
        "rep stosq"
        : "+D"(ptr), "+c"(num_qwords)
        : "a"(0)
        : "memory");

    if (remainder > 0)
    {
        unsigned char *p8 = (unsigned char *)ptr;
        while (remainder--)
        {
            *p8++ = 0;
        }
    }
}

#endif /* AIAOS_KERNEL_MEMORY_H */
