
#include <stdint.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_STMPE610.h>
#include <Wire.h>



#define ILI9341_PINK        0xF81F
#define ILI9341_GREENYELLOW 0xAFE5      /* 173, 255,  47 */
#define ILI9341_LIGHTGREY   0xC618      /* 192, 192, 192 */
#define CS_TOUCH 8
#define CS_SCREEN  10
#define DC 9
#define RST 7
#define HOMESCREEN 0
#define ALARMSCREEN 1

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 150
#define TS_MINY 130
#define TS_MAXX 3800
#define TS_MAXY 4000
#define DEFAULT_ECG_MAX 98
#define DEFAULT_ECG_MIN 90
#define DEFAULT_SP02_MAX 100
#define DEFAULT_SP02_MIN 95

#define BOXSIZE 60

Adafruit_ILI9341 tft = Adafruit_ILI9341(CS_SCREEN, DC);
Adafruit_STMPE610 ts = Adafruit_STMPE610(CS_TOUCH);

#define HEIGHT tft.width()
#define WIDTH tft.height()

// Defining screen proportions
const int rows = 10;
const int columns = 10;

const int vertical_scale = HEIGHT/rows;
const int horizontal_scale = WIDTH/columns;

#include "interface.h"
#include "ecg.h"

int currentMode = 0; // Change mode
int timeout = 0;
int biasECGMax = 0;
int biasECGMin = 0;
int biasSP02Max = 0;
int biasSP02Min = 0;
int i = 0;

// Keep track of screen inversion and alarm state
volatile int inverted = 0;
volatile int activeAlarm = 0;


/* Build all buttons */

Button settings = Button(9,9,2,2,ILI9341_RED,true,"Alarm Settings",&tft);
Button confirm_button = Button(9,1,2,2,ILI9341_GREEN,true,"Confirm",&tft);
Button cancel_button = Button(9,9,2,2,ILI9341_RED,true,"Cancel",&tft);
Button default_button = Button(6,7,2,2,ILI9341_LIGHTGREY,true,"Default Settings",&tft);

/* Build all text boxes */
TextBox title = TextBox(1,3,1,6,ILI9341_BLACK,ILI9341_WHITE,2,true,"   FreePulse Patient Monitor", &tft);
TextBox version = TextBox(2,4,1,4,ILI9341_BLACK,ILI9341_WHITE,2,true," Development: v0.5", &tft);

// Create ECG trace
ECGReadout ecg = ECGReadout(3,2,3,8,14,0,ILI9341_GREENYELLOW,ILI9341_RED,&tft);
ECGReadout spo2 = ECGReadout(6,2,3,8,14,0,ILI9341_BLUE,ILI9341_GREEN,&tft);
/*
 * draw_submenu() - draws color box submenu on left of screen
 */
 
 //Write labels
char Title[] = "FreePulse Patient Monitor";
char Version[] = "Alpha Software v0.1";
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
char confirmLabel[] = "CONFIRM";
char defaultLabel[] = "DEFAULT";
char modeLabel[] = "MODE";
char cancelLabel[] = "CANCEL";

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
    y_coord += 8*textsize;
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

/*
 * showGrid() - 
 * DEVELOPMENT FUNCTION ONLY.
 * Draw gridlines for interface.
 */
void showGrid(void){
    for (int i = 1; i < rows; i += 1) {
        tft.drawFastHLine(1,i*vertical_scale,HEIGHT,ILI9341_LIGHTGREY);
    }
    for (int i = 1; i < columns; i += 1) {
        tft.drawFastVLine(i*horizontal_scale,1,WIDTH,ILI9341_LIGHTGREY);
    }
}

/*
 * gui_setup() - initialize interface
 */
void gui_setup(){
	tft.begin();
	ts.begin();
    clearScreen(ILI9341_BLACK);
	tft.setRotation(1);
}

/*
 * clearScreen(int color) - clear screen with specified color
 */
void clearScreen(int color){
    tft.fillRect(0,0,tft.width(),tft.height(),color);
}

void MainScreenInit(void){
  clearScreen(ILI9341_BLACK);
  title.draw();
  version.draw();
  settings.draw();
  ecg.draw();
  spo2.draw();
}


void SettingsScreenInit(void){
  clearScreen(ILI9341_BLACK);
  confirm_button.draw();
  default_button.draw();
  cancel_button.draw();
}

void fixCoordinates(int* x, int* y){
  int temp = *x;
  *x = *y;
  *y = temp;
}

TS_Point getFixedCoordinates(void){
   if (ts.bufferEmpty()) { return TS_Point(tft.height(), tft.width(), 0); }
   TS_Point p = ts.getPoint();
   while(!ts.bufferEmpty()){
       p = ts.getPoint(); 
   }
   // Scale from 0-4000 to tft.width() using calibration numbers
   p.x = tft.height() - map(p.x, TS_MINX, TS_MAXX, 0, tft.height());
   p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.width());
   int temp = p.x;
   p.x = p.y;
   p.y = temp;
   return p;
}

void toggleInvert(void) {
    cli();
    if (inverted) {
        tft.invertDisplay(false);
        inverted = 0;
    } else {
        tft.invertDisplay(true);
        inverted = 1;
    }
    sei();
    return; 
}

// Interrupts for sampling and alarm system
IntervalTimer sample_timer;
IntervalTimer alarm_timer;

void throwAlarm(void) {
    cli();
    alarm_timer.begin(alarm_isr, 1000000);
    alarm_timer.priority(100);
    activeAlarm = 1;
    sei();
}

void stopAlarm(void) {
    cli();
    alarm_timer.end();
    activeAlarm = 0;
    sei();
}


void setup(void){
  gui_setup();
  ecg.read();
  spo2.read();
  sample_timer.begin(sampling_isr, 150);
  sample_timer.priority(128);
}

void sampling_isr(void) {
    ecg.read();
    spo2.read();
	return;
}

void alarm_isr(void) {
    toggleInvert();
    return;
}

int display_count = 0;
int hr_counter = 0;

void loop(void) {
  /*main screen*/
  if (currentMode == HOMESCREEN){
    MainScreenInit();
    stopAlarm();
    int delay_touch_detection = 10000;
    for (int i = 0; i < delay_touch_detection; i += 1) {
        if (!ts.bufferEmpty()){
            volatile TS_Point tossout = getFixedCoordinates();
        }
    }
    while (currentMode == HOMESCREEN){
  		if (display_count >= 10) {
    		ecg.display_signal();
    		//spo2.display_signal();
    		display_count = 0;
    		if (hr_counter >= 10) {
  	  			/*int hr = ecg.heart_rate();
    			String s_hr = String(hr);
    			hr_counter = 0;
				tft.fillRect(tft.width()-45, 120, 45, 45, ILI9341_BLACK);
  				tft.setCursor(tft.width()-45, 120);
				tft.setTextColor(ILI9341_GREENYELLOW);
  				tft.setTextSize(2);
  				tft.println(s_hr);*/
                //showGrid();
                /*
                if ((hr < DEFAULT_ECG_MIN + biasECGMin) || (hr > DEFAULT_ECG_MAX + biasSP02Max)){
                    if (!activeAlarm) {
                        throwAlarm();
                    }
                }
                else {
                    if (activeAlarm) {
                        stopAlarm(); 
                    }
    		    } */ 
  		}
        else { hr_counter += 1; }
        }
  		else { display_count += 1;} 

	  if (ts.bufferEmpty()) {
		continue;
	  } 
      // Retrieve the touch point
      TS_Point p = getFixedCoordinates();
      if (settings.isTapped(p.x,p.y)){
        currentMode = ALARMSCREEN;
      }
    }
  }
  /*alarm screen*/
  if (currentMode == ALARMSCREEN){
    currentMode = HOMESCREEN;
  }  
}
  
