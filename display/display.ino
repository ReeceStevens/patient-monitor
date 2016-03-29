#include <stdint.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include "Adafruit_RA8875.h"
#include <Wire.h>

#define RA8875_PINK        0xF81F
#define RA8875_GREENYELLOW 0xAFE5      /* 173, 255,  47 */
#define RA8875_LIGHTGREY   0xC618      /* 192, 192, 192 */
#define RA8875_INT 5
#define RA8875_CS 9
#define RA8875_RESET 7
#define HOMESCREEN 0
#define ALARMSCREEN 1

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 150
#define TS_MINY 130
#define TS_MAXX 3800
#define TS_MAXY 4000

// These are for default alarm values and UI
#define DEFAULT_ECG_MAX 98
#define DEFAULT_ECG_MIN 90
#define DEFAULT_SP02_MAX 100
#define DEFAULT_SP02_MIN 95
#define BOXSIZE 60

Adafruit_RA8875 tft = Adafruit_RA8875(RA8875_CS,RA8875_RESET);
uint16_t tx, ty;
const int s_height = 480;
const int s_width = 800;
const float xScale = 1024.0F/s_width;
const float yScale = 1024.0F/s_height;

// Defining screen proportions
const int rows = 10;
const int columns = 10;

const int vertical_scale = s_height / rows;
const int horizontal_scale = s_width / columns;

#include "interface.h"
#include "ecg_revised.h"

int currentMode = 0; // Change mode

// Keep track of screen inversion and alarm state
volatile int inverted = 0;
volatile int activeAlarm = 0;


/* Build UI Buttons */
Button settings = Button(9,9,2,2,RA8875_RED,true,"Alarm Settings",&tft);
Button confirm_button = Button(9,1,2,2,RA8875_GREEN,true,"Confirm",&tft);
Button cancel_button = Button(9,9,2,2,RA8875_RED,true,"Cancel",&tft);
Button default_button = Button(6,7,2,2,RA8875_LIGHTGREY,true,"Default Settings",&tft);

TextBox title = TextBox(1,3,1,3,RA8875_BLACK,RA8875_WHITE,3,true,"FreePulse Patient Monitor v0.9", &tft);

ECGReadout ecg = ECGReadout(2,1,3,8,15,RA8875_BLUE,RA8875_LIGHTGREY,&tft);
ECGReadout ecg2 = ECGReadout(5,1,3,8,16,RA8875_RED,RA8875_LIGHTGREY,&tft);

/*
 * showGrid() - 
 * DEVELOPMENT FUNCTION ONLY.
 * Draw gridlines for interface.
 */
void showGrid(void){
    for (int i = 1; i < rows; i += 1) {
        tft.drawLine(1,i*vertical_scale,s_width-1,i*vertical_scale,RA8875_LIGHTGREY);
    }
    for (int i = 1; i < columns; i += 1) {
        tft.drawLine(i*horizontal_scale,1,i*horizontal_scale,s_height-1,RA8875_LIGHTGREY);
    }
}

void gui_init() {
	tft.begin(RA8875_800x480);

	tft.displayOn(true);
	tft.GPIOX(true);      // Enable TFT - display enable tied to GPIOX
	tft.PWM1config(true, RA8875_PWM_CLK_DIV1024); // PWM output for backlight
	tft.PWM1out(255);

	// With hardware accelleration this is instant
	tft.fillScreen(RA8875_BLACK);
	tft.touchEnable(true);
}

/*
 * clearScreen(int color) - clear screen with specified color
 */
void clearScreen(int color){
    tft.fillScreen(color);
}

void MainScreenInit(void){
  clearScreen(RA8875_BLACK);
  showGrid();
  title.draw();
  settings.draw();
  ecg.draw();
  ecg2.draw();
}


void SettingsScreenInit(void){
  clearScreen(RA8875_BLACK);
  confirm_button.draw();
  default_button.draw();
  cancel_button.draw();
}

void sampling_isr(void) {
    ecg.read();
	ecg2.read();
	return;
}

void display_isr(void) {
	ecg.display_signal();
	ecg2.display_signal();
}

IntervalTimer sample_timer;
IntervalTimer display_timer;

// TODO: try to make a controlled display refresh rate?

void setup(void) {
	Serial.begin(9600);
  	gui_init();
  	clearScreen(RA8875_BLACK);
  	sample_timer.priority(128);
  	display_timer.priority(129);
  	currentMode = HOMESCREEN;
    MainScreenInit();
  	sample_timer.begin(sampling_isr, 500); // 2000 Hz Sampling Rate (device max is 10000)
  	display_timer.begin(display_isr, 800); 
}

void loop(void) {
	if (currentMode == HOMESCREEN) {
		//MainScreenInit();
		int delay_touch_detection = 10000;
    	for (int i = 0; i < delay_touch_detection; i += 1) {
			tft.touchRead(&tx, &ty);
    	}
		// Clear the touch points to prevent double-presses
		tx = 0;
		ty = 0;
		int display_timer = 0;
		int max_time = 200;
        while(currentMode == HOMESCREEN) {
			/*if (display_timer == max_time) {
				display_timer = 0;
		    } else { display_timer += 1; }*/
			//delay(10);
            if (!digitalRead(RA8875_INT) && (tft.touched())) {
				while (!digitalRead(RA8875_INT) && tft.touched()) {
					tft.touchRead(&tx, &ty);
				}
            }
			else {
				// If no touch events, clear the touch points.
				tx = 0;
				ty = 0;
			}
            if (settings.isTapped(tx,ty)){
                clearScreen(RA8875_BLACK);
                currentMode = ALARMSCREEN;
            }
        }
    }
    if (currentMode == ALARMSCREEN) {
        SettingsScreenInit();
		int delay_touch_detection = 10000;
    	for (int i = 0; i < delay_touch_detection; i += 1) {
			tft.touchRead(&tx, &ty);
    	}
		// Clear the touch points to prevent double-presses
		tx = 0;
		ty = 0;
        while (currentMode == ALARMSCREEN) {
            if (!digitalRead(RA8875_INT) && (tft.touched())) {
				while (!digitalRead(RA8875_INT) && tft.touched()) {
					tft.touchRead(&tx, &ty);
				}
            }
			else {
				// If no touch events, clear the touch points.
				tx = 0;
				ty = 0;
			}
            if (cancel_button.isTapped(tx,ty)) {
                clearScreen(RA8875_BLACK);
                currentMode = HOMESCREEN;
            }
        }
    }
}

