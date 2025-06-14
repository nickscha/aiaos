#ifndef AIAOS_KERNEL_MEMORY_H
#define AIAOS_KERNEL_MEMORY_H

typedef struct aiaos_kernel_memory_e820_entry
{
    unsigned long base;
    unsigned long length;
    unsigned int type;
    unsigned int acpi_ext;
} aiaos_kernel_memory_e820_entry;

/* aiaos_linker.ld variables for memory addresses */
extern char _stack_top;                              /* Stack top address as specified in aiaios_linker.ld*/
extern unsigned int _e820_entry_count;               /* E820 entry count as specified in aiaios_linker.ld*/
extern aiaos_kernel_memory_e820_entry _e820_address; /* E820 entry start memory address as specified in aiaios_linker.ld*/

#define AIAOS_KERNEL_MEMORY_E820_MAP_COUNT ((int)_e820_entry_count)
#define AIAOS_KERNEL_MEMORY_E820_MAP_ADDRESS ((aiaos_kernel_memory_e820_entry *)&_e820_address)
#define AIAOS_KERNEL_MEMORY_OFFSET ((unsigned long)&_stack_top) /* Stack Offset. See aiaos_linker.ld */

static void *aiaos_kernel_memory = 0;
static unsigned long aiaos_kernel_memory_size = 0;

void aiaos_kernel_memory_initialize(void)
{
    int i;
    for (i = 0; i < AIAOS_KERNEL_MEMORY_E820_MAP_COUNT; ++i)
    {
        aiaos_kernel_memory_e820_entry *entry = &AIAOS_KERNEL_MEMORY_E820_MAP_ADDRESS[i];

        /* Type 1 means usable RAM */
        if (entry->type == 1 &&
            entry->length >= AIAOS_KERNEL_MEMORY_OFFSET + (1024 * 1024))
        {
            aiaos_kernel_memory = (void *)(unsigned long)(entry->base + AIAOS_KERNEL_MEMORY_OFFSET);
            aiaos_kernel_memory_size = (unsigned long)(entry->length - AIAOS_KERNEL_MEMORY_OFFSET);
            break;
        }
    }
}

void aiaos_kernel_memory_zero(void *ptr, unsigned long size)
{
    unsigned char *p = (unsigned char *)ptr;
    while (size--)
    {
        *p++ = 0;
    }
}

#endif /* AIAOS_KERNEL_MEMORY_H */
