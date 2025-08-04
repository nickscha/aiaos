#ifndef AIAOS_DRIVER_E1000_H
#define AIAOS_DRIVER_E1000_H

#define AIAOS_DRIVER_E1000_VENDOR_ID 0x8086
#define AIAOS_DRIVER_E1000_DEVICE_ID 0x100E

/* MMIO register offsets (not exhaustive) */
#define AIAOS_DRIVER_E1000_REG_CTRL 0x0000
#define AIAOS_DRIVER_E1000_REG_STATUS 0x0008
#define AIAOS_DRIVER_E1000_REG_EECD 0x0010
#define AIAOS_DRIVER_E1000_REG_EERD 0x0014
#define AIAOS_DRIVER_E1000_REG_IMS 0x00D0
#define AIAOS_DRIVER_E1000_REG_RCTL 0x0100
#define AIAOS_DRIVER_E1000_REG_TCTL 0x0400

/* Pointer to MMIO base (mapped by PCI scanner) */
static volatile unsigned int *aiaos_driver_e1000_mmio = 0;

static void aiaos_driver_e1000_write_reg(unsigned long offset, unsigned int value)
{
    aiaos_driver_e1000_mmio[offset / 4] = value;
}

static unsigned int aiaos_driver_e1000_read_reg(unsigned long offset)
{
    return aiaos_driver_e1000_mmio[offset / 4];
}

/* Basic E1000 initialization */
static __inline void aiaos_driver_e1000_init(unsigned long bar0_base)
{
    unsigned int status;

    /* MMIO base is in BAR0, mask out lower 4 bits */
    aiaos_driver_e1000_mmio = (volatile unsigned int *)(bar0_base & ~0xFUL);

    /* Read status to check chip is responding */
    status = aiaos_driver_e1000_read_reg(AIAOS_DRIVER_E1000_REG_STATUS);

    (void)status;

    /* Basic reset - writing to CTRL */
    aiaos_driver_e1000_write_reg(AIAOS_DRIVER_E1000_REG_CTRL, 0x04000000); /* Set Reset */
}

#endif /* AIAOS_DRIVER_E1000_H */
