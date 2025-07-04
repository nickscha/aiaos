#ifndef AIAOS_DRIVER_VGA_H
#define AIAOS_DRIVER_VGA_H

#define AIAOS_DRIVER_VGA_COLUMNS_NUM 80
#define AIAOS_DRIVER_VGA_ROWS_NUM 25
#define AIAOS_DRIVER_VGA_MEMORY (volatile char *)0xB8000

static volatile char *aiaos_driver_vga_buffer = AIAOS_DRIVER_VGA_MEMORY;

typedef enum aiaos_driver_vga_color
{
    AIAOS_DRIVER_VGA_COLOR_BLACK = 0,
    AIAOS_DRIVER_VGA_COLOR_BLUE = 1,
    AIAOS_DRIVER_VGA_COLOR_GREEN = 2,
    AIAOS_DRIVER_VGA_COLOR_CYAN = 3,
    AIAOS_DRIVER_VGA_COLOR_RED = 4,
    AIAOS_DRIVER_VGA_COLOR_MAGENTA = 5,
    AIAOS_DRIVER_VGA_COLOR_BROWN = 6,
    AIAOS_DRIVER_VGA_COLOR_LIGHT_GREY = 7,
    AIAOS_DRIVER_VGA_COLOR_DARK_GREY = 8,
    AIAOS_DRIVER_VGA_COLOR_LIGHT_BLUE = 9,
    AIAOS_DRIVER_VGA_COLOR_LIGHT_GREEN = 10,
    AIAOS_DRIVER_VGA_COLOR_LIGHT_CYAN = 11,
    AIAOS_DRIVER_VGA_COLOR_LIGHT_RED = 12,
    AIAOS_DRIVER_VGA_COLOR_LIGHT_MAGENTA = 13,
    AIAOS_DRIVER_VGA_COLOR_LIGHT_BROWN = 14,
    AIAOS_DRIVER_VGA_COLOR_WHITE = 15,
    AIAOS_DRIVER_VGA_COLOR_COUNT

} aiaos_driver_vga_color;

static char aiaos_driver_vga_make_color(aiaos_driver_vga_color foreground, aiaos_driver_vga_color background)
{
    return (char)(foreground | background << 4);
}

static void aiaos_driver_vga_clear_screen(void)
{
    int i;

    for (i = 0; i < AIAOS_DRIVER_VGA_COLUMNS_NUM * AIAOS_DRIVER_VGA_ROWS_NUM * 2; ++i)
    {
        aiaos_driver_vga_buffer[i] = '\0';
    }
}

void aiaos_driver_vga_clear_screen_row(int row)
{
    int i;

    if (row < 0 || row >= AIAOS_DRIVER_VGA_ROWS_NUM)
    {
        return;
    }

    for (i = 0; i < AIAOS_DRIVER_VGA_COLUMNS_NUM * 2; ++i)
    {
        aiaos_driver_vga_buffer[(AIAOS_DRIVER_VGA_COLUMNS_NUM * 2) * row + i] = '\0';
    }
}

static void aiaos_driver_vga_write_string(const char *str, int row_offset, int col_offset, char color)
{
    int offset;
    int i = 0;

    if (row_offset < 0 || row_offset >= AIAOS_DRIVER_VGA_ROWS_NUM || col_offset < 0 || col_offset >= AIAOS_DRIVER_VGA_COLUMNS_NUM)
    {
        return;
    }

    offset = (row_offset * AIAOS_DRIVER_VGA_COLUMNS_NUM + col_offset) * 2;

    while (str[i] != '\0' && (row_offset * AIAOS_DRIVER_VGA_COLUMNS_NUM + col_offset + i) < (AIAOS_DRIVER_VGA_COLUMNS_NUM * AIAOS_DRIVER_VGA_ROWS_NUM))
    {
        aiaos_driver_vga_buffer[offset + i * 2] = str[i];
        aiaos_driver_vga_buffer[offset + i * 2 + 1] = color;
        i++;
    }
}

#endif /* AIAOS_DRIVER_VGA_H */
