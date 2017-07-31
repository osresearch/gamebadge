
#ifndef __FASTMEM_H__
#define __FASTMEM_H__


#include "defs.h"
#include "mem.h"

#define MEM_DEBUG 0

static byte readb(addr a)
{
	byte *p = mbc.rmap[a>>12];
	byte x;
	if (p)
		x = p[a];
	else
		x = mem_read(a);

	if(MEM_DEBUG) ets_printf("readb %04x -> %02x\n", a, x);
	return x;
}

static void writeb(addr a, byte b)
{
	byte *p = mbc.wmap[a>>12];
	if(MEM_DEBUG) ets_printf("writeb %04x <- %02x\n", a, b);

	if (p)
		p[a] = b;
	else
		mem_write(a, b);
}

static uint16_t readw(addr a)
{
	uint16_t x;
	if ((a+1) & 0xfff)
	{
		byte *p = mbc.rmap[a>>12];
		if (p)
		{
			x = p[a] | ((uint16_t)p[a+1]) << 8;
		} else {
			x = mem_read(a) | ((uint16_t) mem_read(a+1)) << 8;
		}
	} else {
		x = mem_read(a) | ((uint16_t) mem_read(a+1)) << 8;
	}

	if(MEM_DEBUG) ets_printf("readw %04x -> %04x\n", a, x);
	return x;
}

static void writew(addr a, uint16_t w)
{
	if(MEM_DEBUG) ets_printf("writew %04x <- %04x\n", a, w);

	if ((a+1) & 0xfff)
	{
		byte *p = mbc.wmap[a>>12];
		if (p)
		{
			p[a] = w;
			p[a+1] = w >> 8;
			return;
		}
	}
	mem_write(a, w);
	mem_write(a+1, w>>8);
}

static byte readhi(addr a)
{
	return readb(a | 0xff00);
}

static void writehi(addr a, byte b)
{
	writeb(a | 0xff00, b);
}

#if 0
static byte readhi(int a)
{
	byte (*rd)() = hi_read[a];
	return rd ? rd(a) : (ram->hi[a] | himask[a]);
}

static void writehi(int a, byte b)
{
	byte (*wr)() = hi_write[a];
	if (wr) wr(a, b);
	else ram->hi[a] = b & ~himask[a];
}
#endif


#endif
