#ifndef __DEFS_H__
#define __DEFS_H__

#include <stdint.h>


//#ifdef IS_LITTLE_ENDIAN
#if 1
#define LO 0
#define HI 1
#else
#define LO 1
#define HI 0
#endif


typedef uint8_t byte;
typedef uint16_t addr; // 16-bit address space for the gameboy

/* stuff from main.c ... */
#define die(fmt, ...) do { \
	ets_printf("%s:%d died: " fmt "\n", __FILE__, __LINE__, ## __VA_ARGS__); \
	vTaskDelay(1000); \
} while(1)

//void die(char *fmt, ...);
void doevents();



#endif

