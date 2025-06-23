#ifndef AIAOS_PCI_H
#define AIAOS_PCI_H

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC
#define PCI_ENABLE_BIT 0x80000000UL
#define PCI_MAX_BUS 255
#define PCI_MAX_DEVICE 255
#define PCI_MAX_FUNCTION 8

typedef struct aiaos_pci_device
{
    unsigned char bus;
    unsigned char device;
    unsigned char function;
    unsigned short vendor_id;
    unsigned short device_id;
    unsigned char class_code;
    unsigned char subclass;
    unsigned char prog_if;
    unsigned char header_type;

} aiaos_pci_device;

static aiaos_pci_device aiaos_pci_devices[PCI_MAX_DEVICE];
static unsigned int aiaos_pci_devices_count = 0;

static void aiaos_pci_outl(unsigned short port, unsigned int val)
{
    __asm __volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

static unsigned int aiaos_pci_inl(unsigned short port)
{
    unsigned int ret;
    __asm __volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static unsigned long aiaos_pci_config_read32(unsigned char bus, unsigned char device,
                                             unsigned char function, unsigned char offset)
{
    unsigned long address = PCI_ENABLE_BIT |
                            ((unsigned long)bus << 16) |
                            ((unsigned long)device << 11) |
                            ((unsigned long)function << 8) |
                            (offset & 0xFC);

    aiaos_pci_outl(PCI_CONFIG_ADDRESS, (unsigned int)address);
    return aiaos_pci_inl(PCI_CONFIG_DATA);
}

static void aiaos_pci_init(void)
{
    unsigned char bus;
    unsigned char device;
    unsigned char function;

    for (bus = 0; bus < PCI_MAX_BUS; ++bus)
    {
        for (device = 0; device < PCI_MAX_DEVICE; ++device)
        {
            for (function = 0; function < PCI_MAX_FUNCTION; ++function)
            {

                unsigned long data = aiaos_pci_config_read32(bus, device, function, 0x00);
                unsigned long class_info;
                unsigned short vendor_id = (unsigned short)(data & 0xFFFF);
                unsigned short device_id;
                unsigned char header_type;
                aiaos_pci_device current_device;

                if (vendor_id == 0xFFFF)
                {
                    continue;
                }

                device_id = (unsigned short)((data >> 16) & 0xFFFF);
                class_info = aiaos_pci_config_read32(bus, device, function, 0x08);
                header_type = (unsigned char)((aiaos_pci_config_read32(bus, device, function, 0x0C) >> 16) & 0xFF);

                current_device.bus = bus;
                current_device.device = device;
                current_device.function = function;
                current_device.vendor_id = vendor_id;
                current_device.device_id = device_id;
                current_device.class_code = (unsigned char)((class_info >> 24) & 0xFF);
                current_device.subclass = (unsigned char)((class_info >> 16) & 0xFF);
                current_device.prog_if = (unsigned char)((class_info >> 8) & 0xFF);
                current_device.header_type = header_type;

                aiaos_pci_devices[aiaos_pci_devices_count++] = current_device;
            }
        }
    }
}

#endif /* AIAOS_PCI_H */
