#include <stdint.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_STMPE610.h>
#include <Wire.h>

#include "interface.h"
#include "ecg.h"

#define ILI9341_PINK        0xF81F
#define ILI9341_GREENYELLOW 0xAFE5      /* 173, 255,  47 */
#define ILI9341_LIGHTGREY   0xC618      /* 192, 192, 192 */
#define CS_TOUCH 8
#define CS_SCREEN  10
#define DC 9
#define RST 7

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

int currentMode = 0; // Change mode
int timeout = 0;
int biasECGMax = 0;
int biasECGMin = 0;
int biasSP02Max = 0;
int biasSP02Min = 0;

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
Button confirm_button = Button(0,200,BOXSIZE,BOXSIZE,ILI9341_LIGHTGREY,true,&tft);
Button default_button = Button(BOXSIZE,200,BOXSIZE,BOXSIZE,ILI9341_LIGHTGREY,true,&tft);
Button mode_button = Button(BOXSIZE*2,200,BOXSIZE,BOXSIZE,ILI9341_LIGHTGREY,true,&tft);
Button cancel_button = Button(BOXSIZE*3,200,BOXSIZE,BOXSIZE,ILI9341_LIGHTGREY,true,&tft);

// Create settings page buttons
Button ECGPlus = Button(30,70,18,18,ILI9341_BLACK,true,&tft);
Button ECGMinus = Button(100,70,18,18,ILI9341_BLACK,true,&tft);
Button ECGPlus1 = Button(180,70,18,18,ILI9341_BLACK,true,&tft);
Button ECGMinus1 = Button(250,70,18,18,ILI9341_BLACK,true,&tft);
Button SP02Plus = Button(30,120,18,18,ILI9341_WHITE,true,&tft);
Button SP02Minus = Button(100,120,18,18,ILI9341_WHITE,true,&tft);
Button SP02Plus1 = Button(180,120,18,18,ILI9341_WHITE,true,&tft);
Button SP02Minus1 = Button(250,120,18,18,ILI9341_WHITE,true,&tft);


// Create ECG trace
ECGReadout ecg = ECGReadout(10,50,tft.height() - BOXSIZE, 100, 15 , 0, &tft);
TS_Point p;

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
  createHLabel(BOXSIZE + 55, 125, sp02Title, 1, ILI9341_CYAN);
  createVLabel(0, 141, sp02Label, 1, ILI9341_CYAN);
  createVLabel(tft.width()-50, 142, satLabel, 1, ILI9341_CYAN);
  tft.setCursor(tft.width()-40, 146);
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

void alarm_button_setup(void){
  alarm_button.draw();
  createHLabel(265, 210, alarmLabel, 1, ILI9341_BLACK);
  createHLabel(265, 220, settingsLabel, 1, ILI9341_BLACK);
}

void confirm_button_setup(void){
  confirm_button.draw();
  createHLabel(9, 217, confirmLabel, 1, ILI9341_BLACK);
}

void default_button_setup(void){
  default_button.draw();
  createHLabel(BOXSIZE + 10, 217, defaultLabel, 1, ILI9341_BLACK);
}

void mode_button_setup(void){
  mode_button.draw();
  createHLabel(BOXSIZE*2 + 20, 217, modeLabel, 1, ILI9341_BLACK);
}

void cancel_button_setup(void){
  cancel_button.draw();
  createHLabel(BOXSIZE*3 + 14, 217, cancelLabel, 1, ILI9341_BLACK);
}

void ECGSettings_setup(void){
  /* MINIMUM VALUE */
  if (DEFAULT_ECG_MIN + biasECGMin < 100){
    tft.setCursor(63, 72);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_GREEN);
    tft.printf("%d", DEFAULT_ECG_MIN + biasECGMin);
  }
  if (DEFAULT_ECG_MIN + biasECGMin > 99){
    tft.setCursor(56, 72);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_GREEN);
    tft.printf("%d", DEFAULT_ECG_MIN + biasECGMin);
  }
  createVLabel(0, 55, ecgLabel, 2, ILI9341_GREEN);
  ECGPlus.draw();
  ECGMinus.draw();
  createHLabel(34, 72, "-", 2, ILI9341_WHITE);
  createHLabel(104, 72, "+", 2, ILI9341_WHITE);
  
  /* MAXIMUM VALUE */
  if (DEFAULT_ECG_MAX + biasECGMax < 100){
    tft.setCursor(213, 72);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_GREEN);
    tft.printf("%d", DEFAULT_ECG_MAX + biasECGMax);
  }
  if (DEFAULT_ECG_MAX + biasECGMax > 99){
    tft.setCursor(206, 72);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_GREEN);
    tft.printf("%d", DEFAULT_ECG_MAX + biasECGMax);
  }
  ECGPlus1.draw();
  ECGMinus1.draw();
  createHLabel(184, 72, "-", 2, ILI9341_WHITE);
  createHLabel(254, 72, "+", 2, ILI9341_WHITE);
}

void SP02Settings_setup(void){
  /* MINIMUM VALUE */
  if (DEFAULT_ECG_MIN + biasECGMin < 100){
    tft.setCursor(63, 72);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_GREEN);
    tft.printf("%d", DEFAULT_ECG_MIN + biasECGMin);
  }
  if (DEFAULT_ECG_MIN + biasECGMin > 99){
    tft.setCursor(56, 72);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_GREEN);
    tft.printf("%d", DEFAULT_ECG_MIN + biasECGMin);
  }
  createVLabel(0, 55, ecgLabel, 2, ILI9341_GREEN);
  ECGPlus.draw();
  ECGMinus.draw();
  createHLabel(34, 72, "-", 2, ILI9341_WHITE);
  createHLabel(104, 72, "+", 2, ILI9341_WHITE);
  
  /* MAXIMUM VALUE */
  if (DEFAULT_ECG_MAX + biasECGMax < 100){
    tft.setCursor(213, 72);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_GREEN);
    tft.printf("%d", DEFAULT_ECG_MAX + biasECGMax);
  }
  if (DEFAULT_ECG_MAX + biasECGMax > 99){
    tft.setCursor(206, 72);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_GREEN);
    tft.printf("%d", DEFAULT_ECG_MAX + biasECGMax);
  }
  ECGPlus1.draw();
  ECGMinus1.draw();
  createHLabel(184, 72, "-", 2, ILI9341_WHITE);
  createHLabel(254, 72, "+", 2, ILI9341_WHITE);
}
/*
 * clearScreen(int color) - clear screen with specified color
 */
void clearScreen(int color){
    tft.fillRect(0,0,tft.width(),tft.height(),color);
}

void MainScreenInit(void){
  clearScreen(ILI9341_BLACK);
  product_title();  
  ECG_setup();
  sp02_setup();
  //temperature_setup();
  alarm_button_setup();
}

void SettingsScreenInit(void){
  clearScreen(ILI9341_BLACK);
  confirm_button_setup();
  default_button_setup();
  mode_button_setup();
  cancel_button_setup();
  tft.drawFastVLine(BOXSIZE, 200, 40, ILI9341_BLACK);
  tft.drawFastVLine(BOXSIZE*2, 200, 40, ILI9341_BLACK);
  tft.drawFastVLine(BOXSIZE*3, 200, 40, ILI9341_BLACK);
  ECGSettings_setup();
}

/*
void TIMER1_OVF_vect() {
    ecg.read();
    PIT_TFLG1 |= 0x1; // clear interrupt flag
} */

/*void interrupt_setup() {
    PIT_MCR = 0x00;
    PIT_LDVAL1 = 0x00001300; // Setup timer 1 for 20 cycles
    PIT_TCTRL1 = 0x2; // enable Timer 1 interrupts
    PIT_TCTRL1 |= 0x1; // start timer
} */

IntervalTimer myTimer;

void fixCoordinates(int* x, int* y){
  int temp = *x;
  *x = *y;
  *y = temp;
}

TS_Point getFixedCoordinates(void){
   TS_Point p = ts.getPoint();
   // Scale from 0-4000 to tft.width() using calibration numbers
   p.x = tft.height() - map(p.x, TS_MINX, TS_MAXX, 0, tft.height());
   p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.width());
   int temp = p.x;
   p.x = p.y;
   p.y = temp;
   return p;
}

void setup(void){
  gui_setup();
  product_title();  
  ECG_setup();
  sp02_setup();
  alarm_button_setup();
  //temperature_setup();
  //settings_setup();
  ecg.read();
  myTimer.begin(isr, 150);
  myTimer.priority(128);
}

int display_count = 0;
int hr_counter = 0;
void dispECGWave(void){
  if (display_count >= 10) {
        ecg.display_signal();
        display_count = 0;
        if (hr_counter >= 10) {
          int hr = ecg.heart_rate();
    	  String s_hr = String(hr);
    	  hr_counter = 0;
	  tft.fillRect(tft.width()-45, 60, 45, 45, ILI9341_BLACK);
  	  tft.setCursor(tft.width()-45, 60);
	  tft.setTextColor(ILI9341_GREENYELLOW);
  	  tft.setTextSize(2);
  	  tft.println(s_hr);
        } 
        else { hr_counter += 1; }
      }
      else {
        display_count += 1;
      }
}
void isr(void) {
  if (currentMode == 0 && confirm_button.isTapped(p.x,p.y)){
        currentMode = 1;
  }
  if (currentMode == 1 && alarm_button.isTapped(p.x,p.y)){
        currentMode = 0;
  }    
}


void loop(void) {
  /*main screen*/
  if (currentMode == 1){
    MainScreenInit();
    while (currentMode == 1){
      dispECGWave();
      // Retrieve the touch point
      p = getFixedCoordinates();
      //TS_Point p = ts.getPoint();
      // Scale from 0-4000 to tft.width() using calibration numbers
      //p.x = tft.height() - map(p.x, TS_MINX, TS_MAXX, 0, tft.height());
      //p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.width());
      //fixCoordinates(&(p.x), &(p.y));
    }
  }
  /*alarm screen*/
  if (currentMode == 0){
    SettingsScreenInit();
    while (currentMode == 0){
      p = getFixedCoordinates();
      //TS_Point p = ts.getPoint();
      // Scale from 0-4000 to tft.width() using calibration numbers
      //p.x = tft.height() - map(p.x, TS_MINX, TS_MAXX, 0, tft.height());
      //p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.width());
      //fixCoordinates(&(p.x), &(p.y));
    }
  }  
}
  
