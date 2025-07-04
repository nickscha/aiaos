#ifndef AIAOS_DRIVER_PCI_H
#define AIAOS_DRIVER_PCI_H

#define AIAOS_DRIVER_PCI_CONFIG_ADDRESS 0xCF8
#define AIAOS_DRIVER_PCI_CONFIG_DATA 0xCFC
#define AIAOS_DRIVER_PCI_ENABLE_BIT 0x80000000UL
#define AIAOS_DRIVER_PCI_MAX_BUS 255
#define AIAOS_DRIVER_PCI_MAX_DEVICE 255
#define AIAOS_DRIVER_PCI_MAX_FUNCTION 8

typedef struct aiaos_driver_pci_device
{
    unsigned char bus;
    unsigned char device;
    unsigned char function;
    unsigned short vendor_id;
    unsigned short device_id;
    unsigned char class_code;
    unsigned char subclass;
    unsigned char prog_if;
    unsigned char revision_id;
    unsigned char header_type;
    unsigned char irq_line;
    unsigned char multi_function;
    unsigned int bar0;

} aiaos_driver_pci_device;

static aiaos_driver_pci_device aiaos_driver_pci_devices[AIAOS_DRIVER_PCI_MAX_DEVICE];
static unsigned int aiaos_driver_pci_devices_count = 0;

static void aiaos_driver_pci_outl(unsigned short port, unsigned int val)
{
    __asm __volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

static unsigned int aiaos_driver_pci_inl(unsigned short port)
{
    unsigned int ret;
    __asm __volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static unsigned long aiaos_driver_pci_config_read32(unsigned char bus, unsigned char device,
                                                    unsigned char function, unsigned char offset)
{
    unsigned long address = AIAOS_DRIVER_PCI_ENABLE_BIT |
                            ((unsigned long)bus << 16) |
                            ((unsigned long)device << 11) |
                            ((unsigned long)function << 8) |
                            (offset & 0xFC);

    aiaos_driver_pci_outl(AIAOS_DRIVER_PCI_CONFIG_ADDRESS, (unsigned int)address);
    return aiaos_driver_pci_inl(AIAOS_DRIVER_PCI_CONFIG_DATA);
}

static void aiaos_driver_pci_init(void)
{
    unsigned char bus;
    unsigned char device;
    unsigned char function;

    for (bus = 0; bus < AIAOS_DRIVER_PCI_MAX_BUS; ++bus)
    {
        for (device = 0; device < AIAOS_DRIVER_PCI_MAX_DEVICE; ++device)
        {
            for (function = 0; function < AIAOS_DRIVER_PCI_MAX_FUNCTION; ++function)
            {
                unsigned long data = aiaos_driver_pci_config_read32(bus, device, function, 0x00);
                unsigned short vendor_id = (unsigned short)(data & 0xFFFF);
                unsigned short device_id;
                unsigned long class_info;
                unsigned long bar0;
                unsigned char header_type;
                unsigned char revision_id;
                unsigned char irq_line;

                aiaos_driver_pci_device current_device;

                if (vendor_id == 0xFFFF)
                {
                    continue;
                }

                device_id = (unsigned short)((data >> 16) & 0xFFFF);
                class_info = aiaos_driver_pci_config_read32(bus, device, function, 0x08);
                revision_id = (unsigned char)(class_info & 0xFF);
                header_type = (unsigned char)((aiaos_driver_pci_config_read32(bus, device, function, 0x0C) >> 16) & 0xFF);

                bar0 = aiaos_driver_pci_config_read32(bus, device, function, 0x10);
                irq_line = (unsigned char)(aiaos_driver_pci_config_read32(bus, device, function, 0x3C) & 0xFF);

                current_device.bus = bus;
                current_device.device = device;
                current_device.function = function;
                current_device.vendor_id = vendor_id;
                current_device.device_id = device_id;
                current_device.class_code = (unsigned char)((class_info >> 24) & 0xFF);
                current_device.subclass = (unsigned char)((class_info >> 16) & 0xFF);
                current_device.prog_if = (unsigned char)((class_info >> 8) & 0xFF);
                current_device.revision_id = revision_id;
                current_device.header_type = header_type;
                current_device.irq_line = irq_line;
                current_device.bar0 = (unsigned int)bar0;
                current_device.multi_function = (header_type & 0x80) ? 1 : 0;

                aiaos_driver_pci_devices[aiaos_driver_pci_devices_count++] = current_device;

                /* If single-function device, no need to continue */
                if (function == 0 && current_device.multi_function == 0)
                {
                    break;
                }
            }
        }
    }
}

static int aiaos_driver_pci_find_device(aiaos_driver_pci_device *out_device, unsigned short vendor_id, unsigned short device_id)
{
    unsigned int i;

    for (i = 0; i < aiaos_driver_pci_devices_count; ++i)
    {
        aiaos_driver_pci_device *dev = &aiaos_driver_pci_devices[i];

        if (dev->vendor_id == vendor_id && dev->device_id == device_id)
        {
            *out_device = *dev;
            return (1);
        }
    }

    return (0);
}

#endif /* AIAOS_DRIVER_PCI_H */
