#include <stdint.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_STMPE610.h>
#include <Wire.h>

#include "interface.h"
#include "ecg.h"

#define ILI9341_GREENYELLOW 0xAFE5      /* 173, 255,  47 */
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
Button alarm_button = Button(260,200,BOXSIZE,BOXSIZE,ILI9341_RED,true,&tft);

// Create ECG trace
ECGReadout ecg = ECGReadout(10,50,tft.height() - BOXSIZE, 100, 15 , 0, &tft);

/*
 * draw_submenu() - draws color box submenu on left of screen
 */
 
 //Write labels
char Title[] = "Texas Engineering World Health";
char Version[] = "Patient Monitor v2.1";
char ecgTitle[] = "ECG/HR (bpm)";
char ecgLabel[] = "ECG";
char sp02Title[] = "sp02 (%Sat.)";
char sp02Label[] = "sp02";
char hrLabel[] = "HR";
char satLabel[] = "Sat";
char tempTitle[] = "Temperature";
char farLabel[] = "Far";
char celLabel[] = "Cel";
char alarmLabel[] = "ALARM";
char settingsLabel[] = "SETTINGS";
/*
 * create vertical labels
 */
void createVLabel(int x_coord, int y_coord, char* label, int textsize, int color){
  tft.setTextSize(textsize);
  tft.setTextColor(color);
  while(*label){
    tft.setCursor(x_coord, y_coord);
    tft.printf("%c", *label);
    label += 1;
    y_coord += 8;
  }
}

/*
 * create horizontal labels
 */
void createHLabel(int x_coord, int y_coord, char* label,int textsize, int color){
  tft.setTextSize(textsize);
  tft.setTextColor(color);
  tft.setCursor(x_coord, y_coord);
  tft.setCursor(x_coord, y_coord);
  while(*label){
    tft.printf("%c", *label);
    label += 1;
  }
}


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
  createHLabel(BOXSIZE + 20, 0, Title, 1, ILI9341_WHITE);
  createHLabel(BOXSIZE + 40, 10, Version, 1, ILI9341_WHITE);
  
  /* create border */
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

/*
 * ECG_setup() - intialize ECG and HR interfaces
 */
void ECG_setup(){
  createHLabel(BOXSIZE + 55, 40, ecgTitle, 1, ILI9341_GREEN);
  createVLabel(0, 56, ecgLabel, 1, ILI9341_GREEN);
  createVLabel(tft.width()-45, 60, hrLabel, 1, ILI9341_GREEN);
  tft.setCursor(tft.width()-30, 60);
  tft.setTextSize(2);
  tft.println("60");
}

/*
 * sp02_setup() - intialize sp02 and % saturation interfaces
 */
void sp02_setup(){
  createHLabel(BOXSIZE + 55, 95, sp02Title, 1, ILI9341_CYAN);
  createVLabel(0, 111, sp02Label, 1, ILI9341_CYAN);
  createVLabel(tft.width()-50, 112, satLabel, 1, ILI9341_CYAN);
  tft.setCursor(tft.width()-40, 116);
  tft.setTextSize(2);
  tft.println("97%"); 
}

void temperature_setup(){
  createHLabel(BOXSIZE + 55, 150, tempTitle, 1, ILI9341_GREENYELLOW);
  createVLabel(30, 171, farLabel, 1, ILI9341_GREENYELLOW);
  createVLabel(tft.width()-120, 171, celLabel, 1, ILI9341_GREENYELLOW);
  tft.setCursor(50, 175);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_GREENYELLOW);
  tft.println("98.6");
  tft.setCursor(tft.width()-100, 175);
  tft.println("37");
}

void settings_setup(){
    alarm_button.draw();
    createHLabel(265, 210, alarmLabel, 1, ILI9341_BLACK);
    createHLabel(265, 220, settingsLabel, 1, ILI9341_BLACK);
}
/*
 * clearScreen(int color) - clear screen with specified color
 */
void clearScreen(int color){
    tft.fillRect(BOXSIZE,0,tft.width()-BOXSIZE,tft.height(),color);
}

void setup(void){
  gui_setup();
  //product_title();  
  ECG_setup();
  //sp02_setup();
  //temperature_setup();
  //settings_setup();
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
  //clearScreen(ILI9341_CYAN);
  ecg.read();
  ecg.display_signal();
  /*int hr = ecg.heart_rate();
  tft.fillRect(tft.width()-90, 60, 90, 40, ILI9341_BLACK);
  tft.setCursor(tft.width()-90, 60);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_MAGENTA);
  String s_hr = String(hr);
  tft.println(s_hr);*/
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
  
