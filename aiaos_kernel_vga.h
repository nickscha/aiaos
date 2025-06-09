#ifndef AIAOS_KERNEL_VGA_H
#define AIAOS_KERNEL_VGA_H

#define AIAOS_KERNEL_VGA_COLUMNS_NUM 80
#define AIAOS_KERNEL_VGA_ROWS_NUM 25
#define AIAOS_KERNEL_VGA_MEMORY (volatile char *)0xB8000

static volatile char *vga_buffer = AIAOS_KERNEL_VGA_MEMORY;

void aiaos_kernel_vga_clear_screen(void)
{
    int i;

    for (i = 0; i < AIAOS_KERNEL_VGA_COLUMNS_NUM * AIAOS_KERNEL_VGA_ROWS_NUM * 2; ++i)
    {
        vga_buffer[i] = '\0';
    }
}

void aiaos_kernel_vga_clear_screen_row(int row)
{
    int i;

    if (row < 0 || row >= AIAOS_KERNEL_VGA_ROWS_NUM)
    {
        return;
    }

    for (i = 0; i < AIAOS_KERNEL_VGA_COLUMNS_NUM * 2; ++i)
    {
        vga_buffer[(AIAOS_KERNEL_VGA_COLUMNS_NUM * 2) * row + i] = '\0';
    }
}

void aiaos_kernel_vga_write_string(const char *str, int row_offset, int col_offset, char color)
{
    int offset;
    int i = 0;

    if (row_offset < 0 || row_offset >= AIAOS_KERNEL_VGA_ROWS_NUM || col_offset < 0 || col_offset >= AIAOS_KERNEL_VGA_COLUMNS_NUM)
    {
        return;
    }

    offset = (row_offset * AIAOS_KERNEL_VGA_COLUMNS_NUM + col_offset) * 2;

    while (str[i] != '\0' && (row_offset * AIAOS_KERNEL_VGA_COLUMNS_NUM + col_offset + i) < (AIAOS_KERNEL_VGA_COLUMNS_NUM * AIAOS_KERNEL_VGA_ROWS_NUM))
    {
        vga_buffer[offset + i * 2] = str[i];
        vga_buffer[offset + i * 2 + 1] = color;
        i++;
    }
}

#endif /* AIAOS_KERNEL_VGA_H */
