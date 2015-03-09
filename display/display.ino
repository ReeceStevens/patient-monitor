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

// Create submenu buttons
Button hr_button = Button(0,0,BOXSIZE,BOXSIZE,ILI9341_RED,true,&tft);
Button sp_button = Button(0,BOXSIZE,BOXSIZE,BOXSIZE,ILI9341_GREEN,true,&tft);
Button temp_button = Button(0,BOXSIZE*2,BOXSIZE,BOXSIZE,ILI9341_BLUE,true,&tft);
Button alarm_button = Button(0,BOXSIZE*3,BOXSIZE,BOXSIZE,ILI9341_MAGENTA,true,&tft);

/*
 * draw_submenu() - draws color box submenu on left of screen
 */
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
}

/*
 * gui_setup() - initialize interface
 */
void gui_setup(){
	tft.begin();
	ts.begin();
	tft.fillScreen(ILI9341_BLACK);
	tft.setRotation(1);
	draw_submenu();
}

void clearScreen(int color){
    tft.fillRect(BOXSIZE,0,tft.width()-BOXSIZE,tft.height(),color);
}

void setup(void){
  gui_setup();
  product_title();  
}

void loop(void) {
  timeout ++;
  // After timeout, wipe message from screen
  if (timeout > 50000) {
     clearScreen(ILI9341_BLACK);
     timeout = 0;
  }
  
  // Touch screen interfacing taken from touchpaint.ino example
  // in the ILI9341 examples directory
  
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
  
  // Do stuff with the coordinates of the touch
  /*
  if (p.y < BOXSIZE){
   if (p.x < BOXSIZE) {
     if (currentMode != 1){
      tft.fillRect(BOXSIZE, 50, tft.width()-BOXSIZE, tft.height(), ILI9341_BLACK);
     tft.setCursor((tft.width()/2) -50, tft.height()/2);
     tft.setTextSize(2);
     tft.setTextColor(ILI9341_MAGENTA);
     tft.println("Alarm Settings"); 
     currentMode = 1;
     timeout = 0;
     }
   } else if (p.x < BOXSIZE*2) {
     if (currentMode != 2) {
     tft.fillRect(BOXSIZE, 50, tft.width()-BOXSIZE, tft.height(), ILI9341_BLACK); 
     tft.setCursor((tft.width()/2) -50, tft.height()/2);
     tft.setTextSize(2);
     tft.setTextColor(ILI9341_BLUE);
     tft.println("Temperature");  
     currentMode = 2;
     timeout = 0;
     }
   } else if (p.x < BOXSIZE*3){
     if (currentMode != 3) {
     tft.fillRect(BOXSIZE, 50, tft.width()-BOXSIZE, tft.height(), ILI9341_BLACK);
     tft.setCursor((tft.width()/2) -50, tft.height()/2);
     tft.setTextSize(2);
     tft.setTextColor(ILI9341_GREEN);
     tft.println("SpO2"); 
     currentMode = 3;
     timeout = 0;
     }
   } else if (p.x < BOXSIZE*4){
     if (currentMode != 4){
     tft.fillRect(BOXSIZE, 50, tft.width()-BOXSIZE, tft.height(), ILI9341_BLACK);
     tft.setCursor((tft.width()/2) -50, tft.height()/2);
     tft.setTextSize(2);
     tft.setTextColor(ILI9341_RED);
     tft.println("Heart Rate"); 
     currentMode = 4; 
     timeout = 0;
     }
   }
   
    
  }*/
  if (temp_button.isTapped(p.x,p.y)){
        if(!temp_button.lastTapped){
            clearScreen(ILI9341_BLUE);
        }
  } else {
    temp_button.lastTapped = false;
  }

}
  
