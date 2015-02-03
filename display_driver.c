#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

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

static const char *device = "/dev/spidev0.0";
static uint32_t mode;
static uint8_t bits = 9;
static uint32_t speed = 500000;
static uint16_t delay;
static uint32_t fd;

static void writeData(uint8_t data)
{
	int ret;
    uint16_t actualData = (uint16_t) data + 0x0100;
	uint16_t tx[] = {
		actualData
	};
	uint16_t rx[ARRAY_SIZE(tx)] = {0, };
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx,
		.rx_buf = (unsigned long)rx,
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

	for (ret = 0; ret < ARRAY_SIZE(tx); ret++) {
		if (!(ret % 6))
			puts("");
		printf("%.3X ", rx[ret]);
	}
	puts("");
}

static void writeCommand(uint8_t command)
{
	int ret;
    uint16_t actualCommand = (uint16_t) command;
	uint16_t tx[] = {
		actualCommand
	};
	uint16_t rx[ARRAY_SIZE(tx)] = {0, };
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx,
		.rx_buf = (unsigned long)rx,
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

	for (ret = 0; ret < ARRAY_SIZE(tx); ret++) {
		if (!(ret % 6))
			puts("");
		printf("%.3X ", rx[ret]);
	}
	puts("");
}
void spiSetup()
{
    int ret = 0;
    fd = open(device, O_RDWR);
    if (fd < 0){
        printf("Can't open device\n");
    }
	/*
	 * bits per word
	 */
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		printf("can't set bits per word\n");

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		printf("can't get bits per word\n");

	printf("Set bits per word\n");

	/*
	 * spi mode
	 */
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
		printf("can't set spi mode\n");

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
		printf("can't get spi mode\n");


	/*
	 * max speed hz
	 */
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		printf("can't set max speed hz\n");

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		printf("can't get max speed hz\n");

	printf("spi mode: 0x%x\n", mode);
	printf("bits per word: %d\n", bits);
	printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);

}

// Screen Specific Functions and Commands
// Modified from Adafruit_ILI9341.cpp
//
uint8_t setAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1){
	writeCommand(CMD_COLUMN_ADDR_SET);
	// Write X Start
	writeData(x0 >> 8);
	writeData(x0 & 0xFF);
	// Write X End
	writeData(x1 >> 8);
	writeData(x1 & 0xFF);

	writeCommand(CMD_PAGE_ADDR_SET);
	// Write Y Start
	writeData(y0 >> 8);
	writeData(y0 & 0xFF);
	// Write Y End
	writeData(y1 >> 8);
	writeData(y1 & 0xFF);

	writeCommand(CMD_MEM_WRITE);

	return 0;
}

uint8_t setColor(uint16_t color){
	writeData(color >> 8);
	writeData(color);
	return 0;
}

uint8_t drawPixel(uint16_t x, uint16_t y, uint16_t color){
	if ((x >= 320) | (y >= 240)) {goto error;}
	setAddressWindow(x, y, x+1, y+1);
	setColor(color);
	
	return 0;
error:
	return 1;
}

uint8_t fillRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color){
	// Check to ensure we don't go out of bounds
	if ((x >= 320) || (y >= 240)) {goto error;}
	if ((x + w - 1) >= 320) { w = 320 - x;}
	if ((y + h - 1) >= 240) { h = 240 - y;}

	setAddressWindow(x, y, x+w-1, y+h-1);
	setColor(color);
	return 0;

error: 
	return 1;
}

uint8_t fillScreen(uint16_t color){
	fillRectangle(0, 0, 320, 240, color);
	return 0;
}

uint8_t screen_init(){
	writeCommand(CMD_DISP_ON);
	writeCommand(CMD_SLEEP_MODE_OFF);
    writeCommand(CMD_DISP_ID);
	writeCommand(CMD_DISP_ON);
	writeCommand(CMD_SLEEP_MODE_OFF);
    printf("Commands written to screen\n");
	return 0;
}

uint8_t screen_shutdown(){
	writeCommand(CMD_DISP_OFF);
	return 0;
}

void screenInitAdafruit(){
    writeCommand(0xEF);
    writeData(0x03);
    writeData(0x80);
    writeData(0x02);

    writeCommand(0xCF);
    writeData(0x00);
    writeData(0xC1);
    writeData(0x30);

    writeCommand(0xED);
    writeData(0x64);
    writeData(0x03);
    writeData(0x12);
    writeData(0x81);

    writeCommand(0xE8);
    writeData(0x85);
    writeData(0x00);
    writeData(0x78);

    writeCommand(0xCB);
    writeData(0x39);
    writeData(0x2C);
    writeData(0x00);
    writeData(0x34);
    writeData(0x02);

    writeCommand(0xF7);
    writeData(0x20);

    writeCommand(0xEA);
    writeData(0x00);
    writeData(0x00);

    writeCommand(0xC0); // Power control
    writeData(0x23);

    writeCommand(0xC1);
    writeData(0x10);

    writeCommand(0xC5); // VCM Control
    writeData(0x3E);
    writeData(0x28);

    writeCommand(0xC7);
    writeData(0x86);

    writeCommand(0x36); // Memory Access Control
    writeData(0x48);

    writeCommand(0x3A);
    writeData(0x55);

    writeCommand(0xB1);
    writeData(0x00);
    writeData(0x18);

    writeCommand(0xB6); // Display function control
    writeData(0x08);
    writeData(0x82);
    writeData(0x27);

    writeCommand(0xF2); // Disable 3Gamma Function
    writeData(0x00);

    writeCommand(CMD_SLEEP_MODE_OFF);
    // Delay and give the screen time
    writeCommand(CMD_DISP_ON);
    return;
    
}

int main(){
    spiSetup();
    screenInitAdafruit();
    //screen_init();
    fillScreen(0xFAFA);
    close(fd);
    return 0;

}
