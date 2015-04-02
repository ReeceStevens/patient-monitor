#include <stdint.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_STMPE610.h>
#include <Wire.h>

#include "interface.h"

#define CS_TOUCH 8
#define CS_SCREEN  10
#define DC 9
#define RST 7

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 150
#define TS_MINY 130
#define TS_MAXX 3800
#define TS_MAXY 4000

#define BOXSIZE 60

int currentMode = 0; // Change mode
int timeout = 0;

Adafruit_ILI9341 tft = Adafruit_ILI9341(CS_SCREEN, DC);
Adafruit_STMPE610 ts = Adafruit_STMPE610(CS_TOUCH);

#define HEIGHT tft.width()
#define WIDTH tft.height()
// NOTE: tft.height() and tft.width() are mixed up due to orientation issues in the Adafruit drivers.
// tft.height() returns the width (x-axis), while tft.width() returns the height (y-axis)
// hence this awkward macro

// Create submenu buttons
Button hr_button = Button(0,0,BOXSIZE,BOXSIZE,ILI9341_RED,true,&tft);
Button sp_button = Button(0,BOXSIZE,BOXSIZE,BOXSIZE,ILI9341_GREEN,true,&tft);
Button temp_button = Button(0,BOXSIZE*2,BOXSIZE,BOXSIZE,ILI9341_BLUE,true,&tft);
Button alarm_button = Button(0,BOXSIZE*3,BOXSIZE,BOXSIZE,ILI9341_MAGENTA,true,&tft);

// Create ECG trace
ECGReadout ecg = ECGReadout(10,60,tft.height(), tft.width()/2, 15 , 500, &tft);

/*
 * draw_submenu() - draws color box submenu on left of screen
 */
 
 //Write labels
char ecgLabel[4] = "ECG";
char satLabel[5] = "sp02";

//old model interface
void draw_submenu() {
	hr_button.draw();
	sp_button.draw();
	temp_button.draw();
	alarm_button.draw();
}

/*
 * product_title() - prints product name and version no.
 */
void product_title(){
  tft.setCursor(BOXSIZE + 20, 0);
  tft.setTextSize(1);
  tft.println("Texas Engineering World Health");
  tft.setCursor(BOXSIZE + 40, 10);
  tft.println("Patient Monitor v1.0");
  tft.drawFastVLine(BOXSIZE + 17, 0, 20, ILI9341_WHITE);
  tft.drawFastHLine(BOXSIZE + 17, 20, 185, ILI9341_WHITE);
  tft.drawFastVLine(BOXSIZE + 202, 0, 20, ILI9341_WHITE);
}

/*
 * gui_setup() - initialize interface
 */
void gui_setup(){
	tft.begin();
	ts.begin();
	tft.fillScreen(ILI9341_BLACK);
	tft.setRotation(1);
	ecg.draw();
	//draw_submenu();
}

void label_setup(int y_coord, char* letter){
  while(*letter){
    tft.setCursor(0,y_coord);
    tft.printf("%c", *letter);
    letter += 1;
    y_coord += 8;
  }
}
void ECG_setup(){
  tft.setCursor(BOXSIZE + 55, 30);
  tft.setTextSize(1);
  tft.println("HR (bpm) - ECG");
}
void clearScreen(int color){
    tft.fillRect(BOXSIZE,0,tft.width()-BOXSIZE,tft.height(),color);
}

void setup(void){
  gui_setup();
  product_title();  
  ECG_setup();
  tft.setTextColor(ILI9341_MAGENTA);
  label_setup(65, ecgLabel);
  tft.setTextColor(ILI9341_MAGENTA);
  label_setup(95, satLabel);
}

void loop(void) {
  //timeout ++;
  // After timeout, wipe message from screen
  /*
  if (timeout > 50000) {
     clearScreen(ILI9341_BLACK);
	 product_title();
     timeout = 0;
  } */
  
  // Touch screen interfacing taken from touchpaint.ino example
  // in the ILI9341 examples directory
  ecg.read();
  // Check if there is touch data
  if (ts.bufferEmpty()){
    return;
  }
  // Retrieve the touch point
  TS_Point p = ts.getPoint();
  while(!ts.bufferEmpty()){
      TS_Point throwaway = ts.getPoint(); 
  }
  // Scale from 0-4000 to tft.width() using calibration numbers
  p.x = tft.height() - map(p.x, TS_MINX, TS_MAXX, 0, tft.height());
  p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.width());
  int temp = p.y;
  p.y = p.x;
  p.x = temp;
  
			// Debugging the analog read
            tft.setCursor((tft.width()/2) -50, tft.height()/2);
     		tft.setTextSize(2);
     		tft.setTextColor(ILI9341_RED);
			int num = analogRead(15);
			String readout = String(num);
			clearScreen(ILI9341_BLACK);
     		tft.println(readout); 
  // Do stuff with the coordinates of the touch
  /* 
  if (temp_button.isTapped(p.x,p.y)){
        if(!temp_button.lastTapped){
			// Debugging the analog read
            tft.setCursor((tft.width()/2) -50, tft.height()/2);
     		tft.setTextSize(2);
     		tft.setTextColor(ILI9341_RED);
			int num = analogRead(15);
			String readout = String(num);
			clearScreen(ILI9341_BLACK);
     		tft.println(readout); 
        }
  } else {
    temp_button.lastTapped = false;
  }
*/
  

}
  
