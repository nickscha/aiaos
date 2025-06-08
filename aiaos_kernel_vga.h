#ifndef AIAOS_KERNEL_VGA_H
#define AIAOS_KERNEL_VGA_H

#define AIAOS_KERNEL_VGA_COLUMNS_NUM 80
#define AIAOS_KERNEL_VGA_ROWS_NUM 25
#define AIAOS_KERNEL_VGA_MEMORY (volatile char *)0xB8000

void aiaos_kernel_vga_clear_screen(void)
{
    int i;
    volatile char *vga_buffer = AIAOS_KERNEL_VGA_MEMORY;

    for (i = 0; i < AIAOS_KERNEL_VGA_COLUMNS_NUM * AIAOS_KERNEL_VGA_ROWS_NUM * 2; ++i)
    {
        vga_buffer[i] = '\0';
    }
}

void aiaos_kernel_vga_write_string(const char *str, int row_offset, int col_offset, char color)
{
    volatile char *vga_buffer = AIAOS_KERNEL_VGA_MEMORY;
    int offset = (row_offset * AIAOS_KERNEL_VGA_COLUMNS_NUM + col_offset) * 2;
    int i = 0;

    while (str[i] != '\0' && (row_offset * AIAOS_KERNEL_VGA_COLUMNS_NUM + col_offset + i) < (AIAOS_KERNEL_VGA_COLUMNS_NUM * AIAOS_KERNEL_VGA_ROWS_NUM))
    {
        vga_buffer[offset + i * 2] = str[i];
        vga_buffer[offset + i * 2 + 1] = color;
        i++;
    }
}

#endif /* AIAOS_KERNEL_VGA_H */
