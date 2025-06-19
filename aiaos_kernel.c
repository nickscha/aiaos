/* aiaos.h - v0.1 - public domain data structures - nickscha 2025

A C89 nostdlib bare metal 64bit operating system.

LICENSE

  Placed in the public domain and also MIT licensed.
  See end of file for detailed license information.

*/
#include "aiaos_kernel_types.h"
#include "aiaos_kernel_memory.h"
#include "aiaos_kernel_vga.h"
#include "aiaos_pci.h"

typedef struct aiaos_perf_type_llu
{
  unsigned long low_part;
  unsigned long high_part;
} aiaos_perf_type_llu;

static double aiaos_perf_type_llu_to_double(aiaos_perf_type_llu a)
{
  return ((double)a.high_part * 4294967296.0 + (double)a.low_part);
}

static unsigned long aiaos_perf_rdtsc(void)
{
  aiaos_perf_type_llu rdtsc_value = {0};
  __asm __volatile("rdtsc" : "=a"(rdtsc_value.low_part), "=d"(rdtsc_value.high_part));
  return ((unsigned long)aiaos_perf_type_llu_to_double(rdtsc_value));
}

void _start_kernel(void)
{
  int aiaos_logo_length = 23;
  int aiaos_logo_col_offset = (AIAOS_KERNEL_VGA_COLUMNS_NUM - aiaos_logo_length) / 2;
  int aiaos_logo_row_offset = 0;
  int aiaos_logo_long_col_offset = (AIAOS_KERNEL_VGA_COLUMNS_NUM - 34) / 2;
  char buf[31];
  int i;
  unsigned long vgaclear_cpu_cycles_start;
  unsigned long vgaclear_cpu_cycles_end;
  unsigned long meminit_cpu_cycles_start;
  unsigned long meminit_cpu_cycles_end;
  unsigned long memzero_cpu_cycles_start;
  unsigned long memzero_cpu_cycles_end;
  unsigned long stack_total;
  unsigned long stack_usage;

  meminit_cpu_cycles_start = aiaos_perf_rdtsc();
  aiaos_kernel_memory_initialize();
  meminit_cpu_cycles_end = aiaos_perf_rdtsc();

  vgaclear_cpu_cycles_start = aiaos_perf_rdtsc();
  aiaos_kernel_vga_clear_screen();
  vgaclear_cpu_cycles_end = aiaos_perf_rdtsc();

  aiaos_kernel_vga_write_string("[mem] zero all memory started", 0, 0, AIAOS_KERNEL_VGA_COLOR_WHITE);
  memzero_cpu_cycles_start = aiaos_perf_rdtsc();
  aiaos_kernel_memory_zero(aiaos_kernel_memory, aiaos_kernel_memory_size);
  memzero_cpu_cycles_end = aiaos_perf_rdtsc();
  aiaos_kernel_vga_clear_screen_row(0);
  aiaos_kernel_vga_write_string("[mem] zero all memory finish", 0, 0, AIAOS_KERNEL_VGA_COLOR_GREEN);

  aiaos_pci_init();

  /* Logo and Header Text */
  aiaos_kernel_vga_write_string("   _   _   _   _   _", aiaos_logo_row_offset++, aiaos_logo_col_offset, AIAOS_KERNEL_VGA_COLOR_GREEN);
  aiaos_kernel_vga_write_string("  / \\ / \\ / \\ / \\ / \\", aiaos_logo_row_offset++, aiaos_logo_col_offset, AIAOS_KERNEL_VGA_COLOR_GREEN);
  aiaos_kernel_vga_write_string(" ( A | I | A | O | S )", aiaos_logo_row_offset++, aiaos_logo_col_offset, AIAOS_KERNEL_VGA_COLOR_GREEN);
  aiaos_kernel_vga_write_string("  \\_/ \\_/ \\_/ \\_/ \\_/", aiaos_logo_row_offset++, aiaos_logo_col_offset, AIAOS_KERNEL_VGA_COLOR_GREEN);
  aiaos_kernel_vga_write_string("Application is an operating system", aiaos_logo_row_offset + 1, aiaos_logo_long_col_offset, AIAOS_KERNEL_VGA_COLOR_GREEN);

  /* General Information */
  aiaos_kernel_vga_write_string("> Info: x86_64 (64bit) v0.1", 8, 0, AIAOS_KERNEL_VGA_COLOR_GREEN);
  aiaos_kernel_vga_write_string("> Hello from AIAOS C89 Kernel", 9, 0, AIAOS_KERNEL_VGA_COLOR_GREEN);

  /* Available Memory Infomration */
  aiaos_kernel_vga_write_string("> Memory  Base:", 10, 0, AIAOS_KERNEL_VGA_COLOR_GREEN);
  aiaos_kernel_vga_write_string((char *)aiaos_kernel_types_ptr_to_string(aiaos_kernel_memory), 10, 16, aiaos_kernel_vga_make_color(AIAOS_KERNEL_VGA_COLOR_WHITE, AIAOS_KERNEL_VGA_COLOR_GREEN));

  aiaos_kernel_types_ultoa(aiaos_kernel_memory_size, buf);
  aiaos_kernel_vga_write_string("> Memory  Size:", 11, 0, AIAOS_KERNEL_VGA_COLOR_GREEN);
  aiaos_kernel_vga_write_string(buf, 11, 16, AIAOS_KERNEL_VGA_COLOR_GREEN);

  aiaos_kernel_types_uint_to_hex(buf, AIAOS_KERNEL_MEMORY_OFFSET, 16);
  aiaos_kernel_vga_write_string(">   Stack Base:", 12, 0, AIAOS_KERNEL_VGA_COLOR_GREEN);
  aiaos_kernel_vga_write_string(buf, 12, 16, AIAOS_KERNEL_VGA_COLOR_GREEN);

  aiaos_kernel_types_ultoa(memzero_cpu_cycles_end - memzero_cpu_cycles_start, buf);
  aiaos_kernel_vga_write_string("> Memz. Cycles:", 13, 0, AIAOS_KERNEL_VGA_COLOR_GREEN);
  aiaos_kernel_vga_write_string(buf, 13, 16, AIAOS_KERNEL_VGA_COLOR_GREEN);

  aiaos_kernel_types_ultoa(vgaclear_cpu_cycles_end - vgaclear_cpu_cycles_start, buf);
  aiaos_kernel_vga_write_string("> VGAc. Cycles:", 14, 0, AIAOS_KERNEL_VGA_COLOR_GREEN);
  aiaos_kernel_vga_write_string(buf, 14, 16, AIAOS_KERNEL_VGA_COLOR_GREEN);

  aiaos_kernel_types_ultoa(meminit_cpu_cycles_end - meminit_cpu_cycles_start, buf);
  aiaos_kernel_vga_write_string("> Memi. Cycles:", 15, 0, AIAOS_KERNEL_VGA_COLOR_GREEN);
  aiaos_kernel_vga_write_string(buf, 15, 16, AIAOS_KERNEL_VGA_COLOR_GREEN);

  aiaos_kernel_types_ultoa(aiaos_pci_devices_count, buf);
  aiaos_kernel_vga_write_string(">  PCI devices:", 16, 0, AIAOS_KERNEL_VGA_COLOR_GREEN);
  aiaos_kernel_vga_write_string(buf, 16, 16, AIAOS_KERNEL_VGA_COLOR_GREEN);

  /* PCI Devices */
  aiaos_kernel_vga_write_string("PCI Device List:", 8, 40, AIAOS_KERNEL_VGA_COLOR_GREEN);
  aiaos_kernel_vga_write_string("vid     did", 9, 40, AIAOS_KERNEL_VGA_COLOR_GREEN);
  aiaos_kernel_vga_write_string("--------------", 10, 40, AIAOS_KERNEL_VGA_COLOR_GREEN);
  for (i = 0; i < (int)aiaos_pci_devices_count; ++i)
  {
    aiaos_pci_device d = aiaos_pci_devices[i];

    aiaos_kernel_types_uint_to_hex(buf, d.vendor_id, 6);
    aiaos_kernel_vga_write_string(buf, 11 + i, 40, AIAOS_KERNEL_VGA_COLOR_GREEN);

    aiaos_kernel_types_uint_to_hex(buf, d.device_id, 6);
    aiaos_kernel_vga_write_string(buf, 11 + i, 40 + 8, AIAOS_KERNEL_VGA_COLOR_GREEN);
  }

  /* Stack usage */
  stack_total = AIAOS_KERNEL_STACK_SIZE;
  aiaos_kernel_types_ultoa(stack_total, buf);
  aiaos_kernel_vga_write_string(">  Stack total:", 17, 0, AIAOS_KERNEL_VGA_COLOR_GREEN);
  aiaos_kernel_vga_write_string(buf, 17, 16, AIAOS_KERNEL_VGA_COLOR_GREEN);

  stack_usage = aiaos_kernel_memory_stack_usage();
  aiaos_kernel_types_ultoa(stack_usage, buf);
  aiaos_kernel_vga_write_string(">   Stack used:", 18, 0, AIAOS_KERNEL_VGA_COLOR_GREEN);
  aiaos_kernel_vga_write_string(buf, 18, 16, AIAOS_KERNEL_VGA_COLOR_GREEN);

  /* Footer */
  aiaos_kernel_vga_write_string("https://github.com/nickscha/aiaos", 23, aiaos_logo_long_col_offset, AIAOS_KERNEL_VGA_COLOR_GREEN);

  for (i = 0; i < AIAOS_KERNEL_VGA_COLOR_COUNT; i++)
  {
    aiaos_kernel_vga_color color = (aiaos_kernel_vga_color)i;
    aiaos_kernel_vga_write_string("#", 24, i, aiaos_kernel_vga_make_color(color, color));
  }
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
