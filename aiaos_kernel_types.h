#ifndef AIAOS_KERNEL_TYPES_H
#define AIAOS_KERNEL_TYPES_H

void aiaos_kernel_types_string_reverse(char *str, int len)
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
static char *aiaos_kernel_types_uint_to_hex(char *buf, unsigned long val, int width)
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

void aiaos_kernel_types_ultoa(unsigned long value, char *buffer)
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
    aiaos_kernel_types_string_reverse(buffer, i);
}

void *aiaos_kernel_types_ptr_to_string(void *ptr)
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

#endif /* AIAOS_KERNEL_TYPES_H */
