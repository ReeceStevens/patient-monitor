// Driver for the SunFounder 2.8" TFT
// Hardware: Raspberry Pi B+, SPI Interface
//
// Date: 1.7.15
// Author: Reece Stevens

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <sys/mman.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

// Useful debug macros
#include "dbg.h"
// Port Definitions from BCM2835 ARM Peripherals
// Datasheet
#define CS      spi
#define FIFO    (spi + 0x4) 
#define CLK     (spi + 0x8)
#define DLEN    (spi + 0xC)
#define LTOH    (spi + 0x10)
#define DC      (spi + 0x14)

#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)

/* Bitfields in CS */
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

// Software commands for display
#define CMD_RST 0x01
#define CMD_DISP_ID 0x04
#define CMD_DISP_STATUS 0x09
#define CMD_DISP_PWR_MODE 0x0A
#define CMD_DISP_MADCTL 0x0B
#define CMD_DISP_PIXEL_FORM 0x0C
#define CMD_DISP_IMG_FORM 0x0D
#define CMD_DISP_SIGNAL_MODE 0x0E
#define CMD_DISP_SELF_DIAG 0x0F
#define CMD_SLEEP_MODE_ON 0x10
#define CMD_SLEEP_MODE_OFF 0x11
#define CMD_PARTIAL_MODE_ON 0x12
#define CMD_NORM_DISP_MODE_ON 0x13
#define CMD_DISP_INVRT_OFF 0x20
#define CMD_DISP_INVRT_ON 0x21
#define CMD_GAMMA_SET 0x26
#define CMD_DISP_OFF 0x28
#define CMD_DISP_ON 0x29
#define CMD_COLUMN_ADDR_SET 0x2A
#define CMD_PAGE_ADDR_SET 0x2B
#define CMD_MEM_WRITE 0x2C
#define CMD_COLOR_SET 0x2D
#define CMD_MEM_READ 0x2E
#define CMD_PARTIAL_AREA 0x30
#define CMD_VERT_SCROLL_DEF 0x33
#define CMD_TEARING_EFFECT_LINE_OFF 0x34
#define CMD_TEARING_EFFECT_LINE_ON 0x35
#define CMD_MEM_ACCESS_CTL 0x36
#define CMD_VERT_SCROLL_START_ADDR 0x37
#define CMD_IDLE_MODE_OFF 0x38
#define CMD_IDLE_MODE_ON 0x39
#define CMD_PIXEL_FORMAT_SET 0x3A
#define CMD_WRITE_MEM_CONTINUE 0x3C
#define CMD_READ_MEM_CONTINUE 0x3E
#define CMD_SET_TEAR_SCANLINE 0x44
#define CMD_GET_SCANLINE 0x45
#define CMD_WRITE_DISP_BRIGHTNESS 0x51
#define CMD_READ_DISP_BRIGHTNESS 0x52
#define CMD_WRITE_CTRL_DISP 0x53
#define CMD_READ_CTRL_DISP 0x54
#define CMD_WRITE_CONTENT_ADAPTIVE_BRIGHT_CTRL 0x55
#define CMD_READ_CONTENT_ADAPTIVE_BRIGHT_CTRL 0x56
#define CMD_WRITE_CABC_MIN_BRIGHTNESS 0x5E
#define CMD_READ_CABC_MIN_BRIGHTNESS 0x5F
#define CMD_READ_ID1 0xDA
#define CMD_READ_ID2 0xDB
#define CMD_READ_ID3 0xDC
#define CMD_BACKLIGHT_CTRL_1 0xB8
#define CMD_BACKLIGHT_CTRL_2 0xB8
#define CMD_BACKLIGHT_CTRL_3 0xB8
#define CMD_BACKLIGHT_CTRL_4 0xB8
#define CMD_BACKLIGHT_CTRL_5 0xB8
#define CMD_BACKLIGHT_CTRL_6 0xB8
#define CMD_BACKLIGHT_CTRL_7 0xB8
#define CMD_BACKLIGHT_CTRL_8 0xB8
#define CMD_POWER_CTRL_1 0xC0
#define CMD_POWER_CTRL_2 0xC1
#define CMD_INTERFACE_CTRL 0xF6



// Pointer for I/O access
volatile unsigned *gpio;
volatile unsigned *spi;

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

#define MOSI 10
#define MISO 9
#define CE 8
#define SCLK 11

int mem_fd;
void *gpio_map;
void *spi_map;

static const char *device = "/dev/spidev0.0";
static uint32_t mode;
static uint8_t bits = 9;
static uint32_t speed = 500000;
static uint16_t delay;

/*
 * setupio() - This function initializes a memory region to access GPIO
 * Parameters - none
 *
 * */
int setupio(){
    // open /dev/mem
    
    // NOTE: If you're getting a segfault, don't forget to chmod +rw /dev/mem
    // so that you can access the gpio and spi registers
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
    // mmap SPI
    spi_map = mmap(
        NULL,
        0x20, 
        PROT_READ|PROT_WRITE,
        MAP_SHARED,
        mem_fd,
        (GPIO_BASE+0x4000)
    );

    close(mem_fd); // close after mmap is complete
    
    if (gpio_map == MAP_FAILED) {
        printf("gpio mmap error\n");
        goto error;
    }

    if (spi_map == MAP_FAILED) {
        printf("spi mmap error\n");
        goto error;
    }

    gpio = (volatile unsigned *)gpio_map;
    printf("gpio pins set up\n");
    spi = (volatile unsigned *)spi_map;
    printf("spi registers set up\n");
    
    return 0;

error:
    return -1;
}

uint8_t led_heartbeat_setup(void){
    INP_GPIO(21);
    OUT_GPIO(21);
    GPIO_SET = 1<<21;
    return 0;
}

uint8_t spi_setup(void){
    int rc = setupio();
    if (rc) {
        goto error;
    }

    uint8_t i;
    // set gpio pins 7-11 to alt to use SPI interface
    // Note: these are pins 7-11 according to BCM numbering.
    // This isn't the actual order they appear on the board.
    for (i = 7; i <= 11; i++){
        INP_GPIO(i);
        SET_GPIO_ALT(i, 0);
    }

    // Initialize SPI for usage
    uint32_t cs = 0;
    // CS register setup: Chip 0, CPHA 1, CPOL 1, CSPOL 0, LoSSI mode.
    cs =  SPI_CS_CPHA | SPI_CS_CPOL | SPI_CS_CPOL | SPI_CS_LEN ; 
    // Write to the register
    *(CS) = cs;
    
    // Clock divider; 0 -> clock speed of 65536
    uint32_t cdiv = 0;
    *(CLK) = cdiv; 
    printf("setup didn't fail");
    return 0;

error:
    printf("spi setup failed.");
    return 1;
}

uint8_t spi_setup_test(int fd){
    mode = 0x0;
    fd = open(device, O_RDWR);
    if (fd < 0) {
        printf("Unable to open spidev0.0\n");
        goto error;
    }

    int ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1) {
        printf("Unable to write bits per word\n");
        goto error;
    }
    bits = 1;
    ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
    if (ret == -1) {
        printf("Unable to read bits per word\n");
        goto error;
    }

	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    if (ret == -1) {
        printf("Unable to write max speed\n");
        goto error;
    }
	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
    if (ret == -1) {
        printf("Unable to read max speed\n");
        goto error;
    }

    ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
    if (ret == -1) {
        printf("Unable to read mode\n");
        goto error;
    }
    ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
    if (ret == -1) {
        printf("Unable to write mode\n");
        goto error;
    }

	printf("spi mode: 0x%x\n", mode);
	printf("bits per word: %d\n", bits);
	printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);
    
    return 0;
error:
    return 1;
}

void transfer(int fd, uint8_t msg_length, uint16_t tx_init)
{
	int ret;
    uint16_t tx[] = {tx_init};
	uint16_t rx[ARRAY_SIZE(tx)] = {0, };
	struct spi_ioc_transfer tr = { .tx_buf = (unsigned long)tx, .rx_buf = (unsigned long)rx,
		.len = ARRAY_SIZE(tx),
		.delay_usecs = delay,
		.speed_hz = speed,
		.bits_per_word = bits,
	};

	if (mode & SPI_TX_QUAD)
		tr.tx_nbits = 4;
	else if (mode & SPI_TX_DUAL)
		tr.tx_nbits = 2;
	if (mode & SPI_RX_QUAD)
		tr.rx_nbits = 4;
	else if (mode & SPI_RX_DUAL)
		tr.rx_nbits = 2;
	if (!(mode & SPI_LOOP)) {
		if (mode & (SPI_TX_QUAD | SPI_TX_DUAL))
			tr.rx_buf = 0;
		else if (mode & (SPI_RX_QUAD | SPI_RX_DUAL))
			tr.tx_buf = 0;
	}

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		printf("can't send spi message\n");
        return;

    // This for loop is nonsensical, but I'm keeping it
    // for testing purposes
	for (ret = 0; ret < ARRAY_SIZE(tx) ; ret++) {
		if (!(ret % 6))
			puts("");
		printf("%.3X ", rx[ret]);
	}
    fflush(stdout);
	puts("");
}

void writeCommand(uint16_t command, int fd){
    uint16_t tx = command;
    transfer(fd, 1, tx);

}

void writeData(uint16_t data, int fd){
    uint16_t tx = data + 0x100;
    transfer(fd, 1, tx);
}

/*
 * write_data(data) - Write a uint16_t number to the transmit
 * FIFO of the SPI interface.
 * Parameters - data (uint16_t)
 *
 */
uint8_t write_data(uint16_t data){
    // Set TA
    *(CS) |= 0x080;
  
    // Make first bit high to indicate data 
    data = data + 0x100;

    // If TXD = 0, transfer fifo is full
    if (!(*(CS) & 0x40000)){
       printf("fifo full");
       goto error; 
    }
    // Write data to the SPI_FIFO register
    *(FIFO) = data; 

    // Wait for tranfer to finish (DONE = 1)
    while (!(*(CS) & 0x10000)){}
    // Clear TA
    *(CS) &= ~0x080;

    return 0;
    
error:
    return 1;
}

/*
 * write_command(command) - Write a uint16_t number to the transmit
 * FIFO of the SPI interface.
 * Parameters - command (uint16_t)
 *
 */
uint8_t write_command(uint16_t command){
    // Set TA
    *(CS) |= 0x080;

    // MSB must stay a 0 to register as a command

    // If TXD = 0, transfer fifo is full
    if (!(*(CS) & 0x40000)){
       printf("fifo full");
       goto error; 
    }
    // Write data to the SPI_FIFO register
    *(FIFO) = command; 

    // Wait for tranfer to finish (DONE = 1)
    while (!(*(CS) & 0x10000)){}
    // Clear TA
    *(CS) &= ~0x080;

    return 0;
    
error:
    return 1;
}


// Modified from Adafruit_ILI9341.cpp
//
uint8_t setAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, int fd){
	writeCommand(CMD_COLUMN_ADDR_SET,fd);
	// Write X Start
	writeData(x0 >> 8, fd);
	writeData(x0 & 0xFF, fd);
	// Write X End
	writeData(x1 >> 8, fd);
	writeData(x1 & 0xFF, fd);

	writeCommand(CMD_PAGE_ADDR_SET, fd);
	// Write Y Start
	writeData(y0 >> 8, fd);
	writeData(y0 & 0xFF, fd);
	// Write Y End
	writeData(y1 >> 8, fd);
	writeData(y1 & 0xFF, fd);

	writeCommand(CMD_MEM_WRITE, fd);

	return 0;
}

uint8_t setColor(uint16_t color, int fd){
	writeData(color >> 8, fd);
	writeData(color, fd);
	return 0;
}

uint8_t drawPixel(uint16_t x, uint16_t y, uint16_t color, int fd){
	if ((x >= 320) | (y >= 240)) {goto error;}
	setAddressWindow(x, y, x+1, y+1, fd);
	setColor(color, fd);
	
	return 0;
error:
	return 1;
}

uint8_t fillRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color, int fd){
	// Check to ensure we don't go out of bounds
	if ((x >= 320) || (y >= 240)) {goto error;}
	if ((x + w - 1) >= 320) { w = 320 - x;}
	if ((y + h - 1) >= 240) { h = 240 - y;}

	setAddressWindow(x, y, x+w-1, y+h-1, fd);
	setColor(color, fd);
	return 0;

error: 
	return 1;
}

uint8_t fillScreen(uint16_t color, int fd){
	fillRectangle(0, 0, 320, 240, color, fd);
	return 0;
}

uint8_t screen_init(int fd){
	spi_setup();
	writeCommand(CMD_DISP_ON, fd);
	writeCommand(CMD_SLEEP_MODE_OFF, fd);
    writeCommand(CMD_DISP_ID, fd);
    printf("Commands written to screen\n");
	return 0;
}

uint8_t screen_shutdown(int fd){
	writeCommand(CMD_DISP_OFF, fd);
	return 0;
}

void screenInitAdafruit(int fd){
    writeCommand(0xEF, fd);
    writeData(0x03, fd);
    writeData(0x80, fd);
    writeData(0x02, fd);

    writeCommand(0xCF, fd);
    writeData(0x00, fd);
    writeData(0xC1, fd);
    writeData(0x30, fd);

    writeCommand(0xED, fd);
    writeData(0x64, fd);
    writeData(0x03, fd);
    writeData(0x12, fd);
    writeData(0x81, fd);

    writeCommand(0xE8, fd);
    writeData(0x85, fd);
    writeData(0x00, fd);
    writeData(0x78, fd);

    writeCommand(0xCB, fd);
    writeData(0x39, fd);
    writeData(0x2C, fd);
    writeData(0x00, fd);
    writeData(0x34, fd);
    writeData(0x02, fd);

    writeCommand(0xF7, fd);
    writeData(0x20, fd);

    writeCommand(0xEA, fd);
    writeData(0x00, fd);
    writeData(0x00, fd);

    writeCommand(0xC0, fd); // Power control
    writeData(0x23, fd);

    writeCommand(0xC1, fd);
    writeData(0x10, fd);

    writeCommand(0xC5, fd); // VCM Control
    writeData(0x3E, fd);
    writeData(0x28, fd);

    writeCommand(0xC7, fd);
    writeData(0x86, fd);

    writeCommand(0x36, fd); // Memory Access Control
    writeData(0x48, fd);

    writeCommand(0x3A, fd);
    writeData(0x55, fd);

    writeCommand(0xB1, fd);
    writeData(0x00, fd);
    writeData(0x18, fd);

    writeCommand(0xB6, fd); // Display function control
    writeData(0x08, fd);
    writeData(0x82, fd);
    writeData(0x27, fd);

    writeCommand(0xF2, fd); // Disable 3Gamma Function
    writeData(0x00, fd);

    writeCommand(CMD_SLEEP_MODE_OFF, fd);
    // Delay and give the screen time
    uint32_t delay = 100000;
    while (delay){
        delay --;
    }
    writeCommand(CMD_DISP_ON, fd);
    return;
    
}

int main(){
    setupio();
    int fd = open("/dev/spidev0.0", O_RDWR);
    if (fd < 0) {
        printf("Unable to open spidev0.0\n");
        goto error;
    }
    uint8_t rc = spi_setup_test(fd);
    screenInitAdafruit(fd);
    printf("Adafruit version of setup is complete\n");
    //rc = screen_init(fd);
    //rc = screen_shutdown(fd);
    led_heartbeat_setup();
    if (rc) {
        goto error;
    }
    uint32_t i = 100;
    printf("setup is complete\n");
    while(1){
        fillScreen(0x0000, fd);
	writeCommand(CMD_MEM_WRITE, fd);
        GPIO_CLR = 1<<21;
        //write_command(0x20);
    	printf("passing loop\n");
        while (i) {
            i--;
        }
        i = 100000000;
        fillScreen(0xFFFF, fd);
    	writeCommand(CMD_MEM_WRITE, fd);
    	GPIO_SET = 1<<21;
        //write_command(0x21);
        while (i) {
            i--;
        }
        i = 100000000;
    }
    close(fd);
    return 0;
error:
    close(fd);
    return 1;
}

