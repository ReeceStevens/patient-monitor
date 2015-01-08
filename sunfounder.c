// Driver for the SunFounder 2.8" TFT
// Hardware: Raspberry Pi B+, SPI Interface
//
// Date: 1.7.15
// Author: Reece Stevens

#include <stdio.h>
#include <stdint.h>

// Port Definitions from BCM2835 ARM Peripherals
// Datasheet
#define CS (*((volatile int32_t *) 0x7E204000))
#define FIFO (*((volatile int32_t *) 0x7E204004))
#define CLK (*((volatile int32_t *) 0x7E204008))
#define DLEN (*((volatile int32_t *) 0x7E20400C))
#define LTOH (*((volatile int32_t *) 0x7E204010))
#define DC (*((volatile int32_t *) 0x7E204014))

// Define configuration for clock and other 
// SPI control settings

