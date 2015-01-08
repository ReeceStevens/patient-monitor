// Driver for the SunFounder 2.8" TFT
// Hardware: Raspberry Pi B+, SPI Interface
//
// Date: 1.7.15
// Author: Reece Stevens

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

// Useful debug macros
#include "dbg.h"

// Port Definitions from BCM2835 ARM Peripherals
// Datasheet
#define CS      (*((volatile int32_t *) 0x7E204000))
#define FIFO    (*((volatile int32_t *) 0x7E204004))
#define CLK     (*((volatile int32_t *) 0x7E204008))
#define DLEN    (*((volatile int32_t *) 0x7E20400C))
#define LTOH    (*((volatile int32_t *) 0x7E204010))
#define DC      (*((volatile int32_t *) 0x7E204014))

// Define configuration for clock and other 
// SPI control settings
// Source: spi-bcm2708.c driver

#define SPI_CS_LEN_LONG		0x02000000
#define SPI_CS_DMA_LEN		0x01000000
#define SPI_CS_CSPOL2		0x00800000
#define SPI_CS_CSPOL1		0x00400000
#define SPI_CS_CSPOL0		0x00200000
#define SPI_CS_RXF		0x00100000
#define SPI_CS_RXR		0x00080000
#define SPI_CS_TXD		0x00040000
#define SPI_CS_RXD		0x00020000
#define SPI_CS_DONE		0x00010000
#define SPI_CS_LEN		0x00002000
#define SPI_CS_REN		0x00001000
#define SPI_CS_ADCS		0x00000800
#define SPI_CS_INTR		0x00000400
#define SPI_CS_INTD		0x00000200
#define SPI_CS_DMAEN		0x00000100
#define SPI_CS_TA		0x00000080
#define SPI_CS_CSPOL		0x00000040
#define SPI_CS_CLEAR_RX		0x00000020
#define SPI_CS_CLEAR_TX		0x00000010
#define SPI_CS_CPOL		0x00000008
#define SPI_CS_CPHA		0x00000004
#define SPI_CS_CS_10		0x00000002
#define SPI_CS_CS_01		0x00000001

#define SPI_TIMEOUT_MS	150

#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)

// Pointer for I/O access
volatile unsigned *gpio;

// GPIO setup macros
// Source: Linux Foundation
// Note: always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x, y)
#define BCM2708_PERI_BASE   0x20000000
#define GPIO_BASE   (BCM2708_PERI_BASE + 0x200000) // address of GPIO controller
#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(gpio+((g)/10)) |= (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g, a) *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

#define GPIO_SET *(gpio+7) // sets bits which are 1, ignores bits which are 0
#define GPIO_CLR *(gpio+10) // clears bits which are 1, ignores bits which are 0

#define GET_GPIO(g) (*(gpio+13)&(1<<g)) // 0 if LOW, (1<<g) if HIGH

#define GPIO_PULL *(gpio+37) // Pullup/pulldown
#define GPIO_PULLCLK0 *(gpio+38) // Pull up or pull down clock

int mem_fd;
void *gpio_map;
/*
 * setupio() - This function initializes a memory region to access GPIO
 * Parameters - none
 *
 * */
int setupio(){
    // open /dev/mem
    if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
        printf("Can't open /dev/mem\n");
        goto error;
    }

    // mmap GPIO
    gpio_map = mmap(
        NULL,
        BLOCK_SIZE, 
        PROT_READ|PROT_WRITE,
        MAP_SHARED,
        mem_fd,
        GPIO_BASE
    );

    close(mem_fd); // close after mmap is complete
    
    if (gpio_map == MAP_FAILED) {
        printf("mmap error %d\n", (int)gpio_map);
        goto error;
    }

    gpio = (volatile unsigned *)gpio_map;
    return 0;

error:
    return -1;
}

static void spi_setup(void){
    int rc = setupio();
    if (rc) {
        goto error;
    }

    uint8_t i;
    // set gpio pins 7-11 to alt to use SPI interface
    for (i = 7; i <= 11; i++){
        INP_GPIO(i);
        SET_GPIO_ALT(i, 0);
    }

error:
    printf("spi setup failed.");
}
