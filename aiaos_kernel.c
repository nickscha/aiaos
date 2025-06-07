/* aiaos.h - v0.1 - public domain data structures - nickscha 2025

A C89 nostdlib bare metal 64bit operating system.

LICENSE

  Placed in the public domain and also MIT licensed.
  See end of file for detailed license information.

*/
#define VGA_COLUMNS_NUM 80
#define VGA_ROWS_NUM 25
#define VGA_MEMORY (volatile char *)0xB8000

void vga_clear_screen(void)
{
    int i;
    volatile char *vga_buffer = VGA_MEMORY;

    for (i = 0; i < VGA_COLUMNS_NUM * VGA_ROWS_NUM * 2; ++i)
    {
        vga_buffer[i] = '\0';
    }
}

void vga_write_string(const char *str, int row_offset, int col_offset, char color)
{
    volatile char *vga_buffer = VGA_MEMORY;
    int offset = (row_offset * VGA_COLUMNS_NUM + col_offset) * 2;
    int i = 0;

    while (str[i] != '\0' && (row_offset * VGA_COLUMNS_NUM + col_offset + i) < (VGA_COLUMNS_NUM * VGA_ROWS_NUM))
    {
        vga_buffer[offset + i * 2] = str[i];
        vga_buffer[offset + i * 2 + 1] = color;
        i++;
    }
}

void _start_kernel(void)
{
    int aiaos_logo_length = 23;
    int aiaos_logo_col_offset = (VGA_COLUMNS_NUM - aiaos_logo_length) / 2;
    int aiaos_logo_row_offset = 1;
    int aiaos_logo_long_col_offset = (VGA_COLUMNS_NUM - 34) / 2;

    vga_clear_screen();

    /* Logo and Header Text */
    vga_write_string("   _   _   _   _   _", aiaos_logo_row_offset++, aiaos_logo_col_offset, 0x02);
    vga_write_string("  / \\ / \\ / \\ / \\ / \\", aiaos_logo_row_offset++, aiaos_logo_col_offset, 0x02);
    vga_write_string(" ( A | I | A | O | S )", aiaos_logo_row_offset++, aiaos_logo_col_offset, 0x02);
    vga_write_string("  \\_/ \\_/ \\_/ \\_/ \\_/", aiaos_logo_row_offset++, aiaos_logo_col_offset, 0x02);
    vga_write_string("Application is an operating system", aiaos_logo_row_offset += 2, aiaos_logo_long_col_offset, 0x02);

    /* Information */
    vga_write_string("> Version: 0.1", 12, 0, 0x02);
    vga_write_string("> Hello from 64bit AIAOS with C89 Kernel", 13, 0, 0x02);

    /* Footer */
    vga_write_string("https://github.com/nickscha/aiaos", 23, aiaos_logo_long_col_offset, 0x02);
}

/*
   ------------------------------------------------------------------------------
   This software is available under 2 licenses -- choose whichever you prefer.
   ------------------------------------------------------------------------------
   ALTERNATIVE A - MIT License
   Copyright (c) 2025 nickscha
   Permission is hereby granted, free of charge, to any person obtaining a copy of
   this software and associated documentation files (the "Software"), to deal in
   the Software without restriction, including without limitation the rights to
   use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
   of the Software, and to permit persons to whom the Software is furnished to do
   so, subject to the following conditions:
   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
   ------------------------------------------------------------------------------
   ALTERNATIVE B - Public Domain (www.unlicense.org)
   This is free and unencumbered software released into the public domain.
   Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
   software, either in source code form or as a compiled binary, for any purpose,
   commercial or non-commercial, and by any means.
   In jurisdictions that recognize copyright laws, the author or authors of this
   software dedicate any and all copyright interest in the software to the public
   domain. We make this dedication for the benefit of the public at large and to
   the detriment of our heirs and successors. We intend this dedication to be an
   overt act of relinquishment in perpetuity of all present and future rights to
   this software under copyright law.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
   WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   ------------------------------------------------------------------------------
*/
