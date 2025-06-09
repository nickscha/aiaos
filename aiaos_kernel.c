/* aiaos.h - v0.1 - public domain data structures - nickscha 2025

A C89 nostdlib bare metal 64bit operating system.

LICENSE

  Placed in the public domain and also MIT licensed.
  See end of file for detailed license information.

*/
#include "aiaos_kernel_memory.h"
#include "aiaos_kernel_vga.h"

void reverse(char *str, int len)
{
    int i = 0;
    int j = len - 1;
    char temp;
    while (i < j)
    {
        temp = str[i];
        str[i] = str[j];
        str[j] = temp;
        i++;
        j--;
    }
}

/* Converts a 32-bit or 64-bit unsigned integer to hexadecimal string */
static char *uint_to_hex(char *buf, unsigned long val, int width)
{
    static const char hex[] = "0123456789ABCDEF";
    int i;
    for (i = width - 1; i >= 0; --i)
    {
        buf[i] = hex[val & 0xF];
        val >>= 4;
    }
    return buf + width;
}

void ultoa(unsigned long value, char *buffer)
{
    int i = 0;

    if (value == 0)
    {
        buffer[i++] = '0';
        buffer[i] = '\0';
        return;
    }

    while (value > 0)
    {
        unsigned long digit = value % 10;
        buffer[i++] = (char)('0' + digit);
        value /= 10;
    }

    buffer[i] = '\0';
    reverse(buffer, i);
}

void *ptr_to_string(void *ptr)
{
    static char buffer[19]; /* "0x" + 16 hex digits + null terminator */
    unsigned long value = (unsigned long)(unsigned long *)ptr;
    int i = 0;
    int shift;
    char hex_chars[] = "0123456789abcdef";

    buffer[i++] = '0';
    buffer[i++] = 'x';

    /* Write 16 hex digits, most significant nibble first */
    for (shift = (sizeof(unsigned long) * 8) - 4; shift >= 0; shift -= 4)
    {
        char digit = (char)((value >> shift) & 0xF);
        buffer[i++] = hex_chars[(int)digit];
    }

    buffer[i] = '\0';
    return (void *)buffer;
}

void _start_kernel(void)
{
    int aiaos_logo_length = 23;
    int aiaos_logo_col_offset = (AIAOS_KERNEL_VGA_COLUMNS_NUM - aiaos_logo_length) / 2;
    int aiaos_logo_row_offset = 0;
    int aiaos_logo_long_col_offset = (AIAOS_KERNEL_VGA_COLUMNS_NUM - 34) / 2;
    char buf[31];
    int i;
    unsigned long eh820_usable_count = 0;

    aiaos_kernel_memory_initialize();

    /* TODO: something is wrong with the paging and stack alignment. (1024 * 1024) - 4104) worked (1024 * 1024) - 4103) failed */
    aiaos_kernel_memory_zero(aiaos_kernel_memory, (1024 * 1024) - 4104);

    aiaos_kernel_vga_clear_screen();

    /* Logo and Header Text */
    aiaos_kernel_vga_write_string("   _   _   _   _   _", aiaos_logo_row_offset++, aiaos_logo_col_offset, 0x02);
    aiaos_kernel_vga_write_string("  / \\ / \\ / \\ / \\ / \\", aiaos_logo_row_offset++, aiaos_logo_col_offset, 0x02);
    aiaos_kernel_vga_write_string(" ( A | I | A | O | S )", aiaos_logo_row_offset++, aiaos_logo_col_offset, 0x02);
    aiaos_kernel_vga_write_string("  \\_/ \\_/ \\_/ \\_/ \\_/", aiaos_logo_row_offset++, aiaos_logo_col_offset, 0x02);
    aiaos_kernel_vga_write_string("Application is an operating system", aiaos_logo_row_offset + 1, aiaos_logo_long_col_offset, 0x02);

    /* General Information */
    aiaos_kernel_vga_write_string("> Version: 0.1", 8, 0, 0x02);
    aiaos_kernel_vga_write_string("> Hello from 64bit AIAOS C89 Kernel", 9, 0, 0x02);

    /* Available Memory Infomration */
    aiaos_kernel_vga_write_string(">   Memory Base:", 10, 0, 0x02);
    aiaos_kernel_vga_write_string((char *)ptr_to_string(aiaos_kernel_memory), 10, 17, 0x02);

    ultoa(aiaos_kernel_memory_size, buf);
    aiaos_kernel_vga_write_string(">   Memory Size:", 11, 0, 0x02);
    aiaos_kernel_vga_write_string(buf, 11, 17, 0x02);

    /* E820 Information */
    ultoa((unsigned long)AIAOS_KERNEL_MEMORY_E820_MAP_COUNT, buf);
    aiaos_kernel_vga_write_string("> EH820 Entries:", 12, 0, 0x02);
    aiaos_kernel_vga_write_string(buf, 12, 17, 0x02);

    for (i = 0; i < AIAOS_KERNEL_MEMORY_E820_MAP_COUNT; ++i)
    {
        aiaos_kernel_memory_e820_entry *entry = &AIAOS_KERNEL_MEMORY_E820_MAP_ADDRESS[i];
        if (entry->type == 1)
        {
            uint_to_hex(buf, entry->base, 16);
            aiaos_kernel_vga_write_string("b:", 8 + (int)eh820_usable_count, (AIAOS_KERNEL_VGA_COLUMNS_NUM / 2) - 3 + 1, 0x02);
            aiaos_kernel_vga_write_string(buf, 8 + (int)eh820_usable_count, (AIAOS_KERNEL_VGA_COLUMNS_NUM / 2) - 3 + 1 + 3, 0x02);

            ultoa(entry->length, buf);
            aiaos_kernel_vga_write_string("l_kb:", 8 + (int)eh820_usable_count, (AIAOS_KERNEL_VGA_COLUMNS_NUM / 2) - 3 + 1 + 20, 0x02);
            aiaos_kernel_vga_write_string(buf, 8 + (int)eh820_usable_count, (AIAOS_KERNEL_VGA_COLUMNS_NUM / 2) - 3 + 1 + 20 + 6, 0x02);

            eh820_usable_count++;
        }
    }
    ultoa(eh820_usable_count, buf);
    aiaos_kernel_vga_write_string("> EH820  Usable:", 13, 0, 0x02);
    aiaos_kernel_vga_write_string(buf, 13, 17, 0x02);

    /* Vertical divider */
    for (i = 0; i < 14; ++i)
    {
        aiaos_kernel_vga_write_string("|", 8 + i, (AIAOS_KERNEL_VGA_COLUMNS_NUM / 2) - 4, 0x02);
    }

    /* Footer */
    aiaos_kernel_vga_write_string("https://github.com/nickscha/aiaos", 23, aiaos_logo_long_col_offset, 0x02);
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
