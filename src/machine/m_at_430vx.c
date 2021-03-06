/*
 * 86Box	A hypervisor and IBM PC system emulator that specializes in
 *		running old operating systems and software designed for IBM
 *		PC systems and compatibles from 1981 through fairly recent
 *		system designs based on the PCI bus.
 *
 *		This file is part of the 86Box distribution.
 *
 *		Implementation of the Intel 430VX PCISet chip.
 *
 * Version:	@(#)m_at_430vx.c	1.0.13	2018/04/26
 *
 * Authors:	Sarah Walker, <http://pcem-emulator.co.uk/>
 *		Miran Grca, <mgrca8@gmail.com>
 *
 *		Copyright 2008-2018 Sarah Walker.
 *		Copyright 2016-2018 Miran Grca.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "../86box.h"
#include "../device.h"
#include "../keyboard.h"
#include "../io.h"
#include "../pci.h"
#include "../mem.h"
#include "../memregs.h"
#include "../piix.h"
#include "../intel_flash.h"
#include "../sio.h"
#include "machine.h"


typedef struct
{
    uint8_t	regs[256];
} i430vx_t;


static void
i430vx_map(uint32_t addr, uint32_t size, int state)
{
    switch (state & 3) {
	case 0:
		mem_set_mem_state(addr, size, MEM_READ_EXTERNAL | MEM_WRITE_EXTERNAL);
		break;
	case 1:
		mem_set_mem_state(addr, size, MEM_READ_INTERNAL | MEM_WRITE_EXTERNAL);
		break;
	case 2:
		mem_set_mem_state(addr, size, MEM_READ_EXTERNAL | MEM_WRITE_INTERNAL);
		break;
	case 3:
		mem_set_mem_state(addr, size, MEM_READ_INTERNAL | MEM_WRITE_INTERNAL);
		break;
    }
    flushmmucache_nopc();
}


static void
i430vx_write(int func, int addr, uint8_t val, void *priv)
{
    i430vx_t *dev = (i430vx_t *) priv;

    if (func)
	return;

    if ((addr >= 0x10) && (addr < 0x4f))
	return;

    switch (addr) {
	case 0x00: case 0x01: case 0x02: case 0x03:
	case 0x08: case 0x09: case 0x0a: case 0x0b:
	case 0x0c: case 0x0e:
		return;

	case 0x04: /*Command register*/
		val &= 0x02;
		val |= 0x04;
		break;
	case 0x05:
		val = 0;
		break;

	case 0x06: /*Status*/
		val = 0;
		break;
	case 0x07:
		val &= 0x80;
		val |= 0x02;
		break;

	case 0x59: /*PAM0*/
		if ((dev->regs[0x59] ^ val) & 0xf0) {
			i430vx_map(0xf0000, 0x10000, val >> 4);
			shadowbios = (val & 0x10);
		}
		break;
	case 0x5a: /*PAM1*/
		if ((dev->regs[0x5a] ^ val) & 0x0f)
			i430vx_map(0xc0000, 0x04000, val & 0xf);
		if ((dev->regs[0x5a] ^ val) & 0xf0)
			i430vx_map(0xc4000, 0x04000, val >> 4);
		break;
	case 0x5b: /*PAM2*/
		if ((dev->regs[0x5b] ^ val) & 0x0f)
			i430vx_map(0xc8000, 0x04000, val & 0xf);
		if ((dev->regs[0x5b] ^ val) & 0xf0)
			i430vx_map(0xcc000, 0x04000, val >> 4);
		break;
	case 0x5c: /*PAM3*/
		if ((dev->regs[0x5c] ^ val) & 0x0f)
			i430vx_map(0xd0000, 0x04000, val & 0xf);
		if ((dev->regs[0x5c] ^ val) & 0xf0)
			i430vx_map(0xd4000, 0x04000, val >> 4);
		break;
	case 0x5d: /*PAM4*/
		if ((dev->regs[0x5d] ^ val) & 0x0f)
			i430vx_map(0xd8000, 0x04000, val & 0xf);
		if ((dev->regs[0x5d] ^ val) & 0xf0)
			i430vx_map(0xdc000, 0x04000, val >> 4);
		break;
	case 0x5e: /*PAM5*/
		if ((dev->regs[0x5e] ^ val) & 0x0f)
			i430vx_map(0xe0000, 0x04000, val & 0xf);
		if ((dev->regs[0x5e] ^ val) & 0xf0)
			i430vx_map(0xe4000, 0x04000, val >> 4);
		break;
	case 0x5f: /*PAM6*/
		if ((dev->regs[0x5f] ^ val) & 0x0f)
			i430vx_map(0xe8000, 0x04000, val & 0xf);
		if ((dev->regs[0x5f] ^ val) & 0xf0)
			i430vx_map(0xec000, 0x04000, val >> 4);
		break;
	case 0x72: /*SMRAM*/
		if ((dev->regs[0x72] ^ val) & 0x48)
			i430vx_map(0xa0000, 0x20000, ((val & 0x48) == 0x48) ? 3 : 0);
		break;
    }

    dev->regs[addr] = val;
}


static uint8_t i430vx_read(int func, int addr, void *priv)
{
    i430vx_t *dev = (i430vx_t *) priv;

    if (func)
	return 0xff;

    return dev->regs[addr];
}


static void
i430vx_reset(void *priv)
{
    i430vx_write(0, 0x59, 0x00, priv);
    i430vx_write(0, 0x72, 0x02, priv);
}


static void
i430vx_close(void *p)
{
    i430vx_t *i430vx = (i430vx_t *)p;

    free(i430vx);
}


static void
*i430vx_init(const device_t *info)
{
    i430vx_t *i430vx = (i430vx_t *) malloc(sizeof(i430vx_t));
    memset(i430vx, 0, sizeof(i430vx_t));

    i430vx->regs[0x00] = 0x86; i430vx->regs[0x01] = 0x80; /*Intel*/
    i430vx->regs[0x02] = 0x30; i430vx->regs[0x03] = 0x70; /*82437VX*/
    i430vx->regs[0x04] = 0x06; i430vx->regs[0x05] = 0x00;
    i430vx->regs[0x06] = 0x00; i430vx->regs[0x07] = 0x02;
    i430vx->regs[0x08] = 0x00; /*A0 stepping*/
    i430vx->regs[0x09] = 0x00; i430vx->regs[0x0a] = 0x00; i430vx->regs[0x0b] = 0x06;
    i430vx->regs[0x52] = 0x42; /*256kb PLB cache*/
    i430vx->regs[0x53] = 0x14;
    i430vx->regs[0x56] = 0x52; /*DRAM control*/
    i430vx->regs[0x57] = 0x01;
    i430vx->regs[0x60] = i430vx->regs[0x61] = i430vx->regs[0x62] = i430vx->regs[0x63] = 0x02;
    i430vx->regs[0x64] = 0x02;
    i430vx->regs[0x67] = 0x11;
    i430vx->regs[0x69] = 0x03;
    i430vx->regs[0x70] = 0x20;
    i430vx->regs[0x72] = 0x02;
    i430vx->regs[0x74] = 0x0e;
    i430vx->regs[0x78] = 0x23;

    pci_add_card(0, i430vx_read, i430vx_write, i430vx);

    return i430vx;
}


const device_t i430vx_device =
{
    "Intel 82437VX",
    DEVICE_PCI,
    0,
    i430vx_init, 
    i430vx_close, 
    i430vx_reset,
    NULL,
    NULL,
    NULL,
    NULL
};


void
machine_at_p55tvp4_init(const machine_t *model)
{
    machine_at_common_init(model);
    device_add(&keyboard_ps2_pci_device);

    memregs_init();
    pci_init(PCI_CONFIG_TYPE_1);
    pci_register_slot(0x00, PCI_CARD_SPECIAL, 0, 0, 0, 0);
    pci_register_slot(0x0C, PCI_CARD_NORMAL, 1, 2, 3, 4);
    pci_register_slot(0x0B, PCI_CARD_NORMAL, 2, 3, 4, 1);
    pci_register_slot(0x0A, PCI_CARD_NORMAL, 3, 4, 1, 2);
    pci_register_slot(0x09, PCI_CARD_NORMAL, 4, 1, 2, 3);
    pci_register_slot(0x07, PCI_CARD_SPECIAL, 0, 0, 0, 0);
    device_add(&i430vx_device);
    device_add(&piix3_device);
    w83877f_init();

    device_add(&intel_flash_bxt_device);
}


void
machine_at_i430vx_init(const machine_t *model)
{
    machine_at_common_init(model);
    device_add(&keyboard_ps2_pci_device);

    memregs_init();
    pci_init(PCI_CONFIG_TYPE_1);
    pci_register_slot(0x00, PCI_CARD_SPECIAL, 0, 0, 0, 0);
    pci_register_slot(0x11, PCI_CARD_NORMAL, 2, 3, 4, 1);
    pci_register_slot(0x12, PCI_CARD_NORMAL, 1, 2, 3, 4);
    pci_register_slot(0x14, PCI_CARD_NORMAL, 3, 4, 1, 2);
    pci_register_slot(0x13, PCI_CARD_NORMAL, 4, 1, 2, 3);
    pci_register_slot(0x07, PCI_CARD_SPECIAL, 0, 0, 0, 0);
    device_add(&i430vx_device);
    device_add(&piix3_device);
    um8669f_init();

    device_add(&intel_flash_bxt_device);
}


void
machine_at_p55va_init(const machine_t *model)
{
    machine_at_common_init(model);
    device_add(&keyboard_ps2_pci_device);

    pci_init(PCI_CONFIG_TYPE_1);
    pci_register_slot(0x00, PCI_CARD_SPECIAL, 0, 0, 0, 0);
    pci_register_slot(0x08, PCI_CARD_NORMAL, 1, 2, 3, 4);
    pci_register_slot(0x09, PCI_CARD_NORMAL, 2, 3, 4, 1);
    pci_register_slot(0x0A, PCI_CARD_NORMAL, 3, 4, 1, 2);
    pci_register_slot(0x0B, PCI_CARD_NORMAL, 4, 1, 2, 3);
    pci_register_slot(0x07, PCI_CARD_SPECIAL, 0, 0, 0, 0);
    device_add(&i430vx_device);
    device_add(&piix3_device);
    fdc37c932fr_init();

    device_add(&intel_flash_bxt_device);
}
