#include <stdint.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include "Adafruit_RA8875.h"
//#include <Adafruit_RA8875.h>
//#include <Adafruit_STMPE610.h>
#include <Wire.h>

#define RA8875_PINK        0xF81F
#define RA8875_GREENYELLOW 0xAFE5      /* 173, 255,  47 */
#define RA8875_LIGHTGREY   0xC618      /* 192, 192, 192 */
#define RA8875_INT 5
#define RA8875_CS 9
#define RA8875_RESET 7
//#define CS_TOUCH 8
//#define CS_SCREEN  10
//#define DC 9
//#define RST 7
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

Adafruit_RA8875 tft = Adafruit_RA8875(RA8875_CS,RA8875_RESET);
uint16_t tx, ty;
//Adafruit_RA8875 tft = Adafruit_RA8875(CS_SCREEN, DC);
//Adafruit_STMPE610 ts = Adafruit_STMPE610(CS_TOUCH);

//#define HEIGHT tft.s_width()
//#define WIDTH tft.s_height()

const int s_height = 480;
const int s_width = 800;
//const int s_height = 800;
//const int s_width = 480;

//const int s_height = tft.s_height();
//const int s_width = tft.s_width();

// Defining screen proportions
const int rows = 10;
const int columns = 10;

//const int vertical_scale = 48;
//const int horizontal_scale = 80;

const int vertical_scale = s_height / rows;
const int horizontal_scale = s_width / columns;

#include "interface.h"
//#include "ecg.h"

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

Button settings = Button(9,9,2,2,RA8875_RED,true,"Alarm Settings",&tft);
Button confirm_button = Button(9,1,2,2,RA8875_GREEN,true,"Confirm",&tft);
Button cancel_button = Button(9,9,2,2,RA8875_RED,true,"Cancel",&tft);
Button default_button = Button(6,7,2,2,RA8875_LIGHTGREY,true,"Default Settings",&tft);

/* Build all text boxes */
TextBox title = TextBox(1,3,1,6,RA8875_GREENYELLOW,RA8875_WHITE,1,true,"   FreePulse Patient Monitor", &tft);
TextBox version = TextBox(2,5,1,4,RA8875_BLACK,RA8875_WHITE,1,true," Development: v0.5", &tft);

// OLD CODE


// Create ECG trace
//ECGReadout ecg = ECGReadout(3,2,3,8,14,0,RA8875_GREENYELLOW,RA8875_RED,&tft);
//ECGReadout spo2 = ECGReadout(6,2,3,8,14,0,RA8875_BLUE,RA8875_GREEN,&tft);
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
        //tft.drawFastHLine(1,i*vertical_scale,s_width-1,RA8875_LIGHTGREY);
        tft.drawLine(1,i*vertical_scale,s_width-1,i*vertical_scale,RA8875_LIGHTGREY);
    }
    for (int i = 1; i < columns; i += 1) {
        //tft.drawFastVLine(i*horizontal_scale,1,s_height-1,RA8875_LIGHTGREY);
        tft.drawLine(i*horizontal_scale,1,i*horizontal_scale,s_height-1,RA8875_LIGHTGREY);
    }
}

/*
 * gui_setup() - initialize interface
 */
void gui_setup(){
	tft.begin(RA8875_800x480);

  tft.displayOn(true);
  tft.GPIOX(true);      // Enable TFT - display enable tied to GPIOX
  tft.PWM1config(true, RA8875_PWM_CLK_DIV1024); // PWM output for backlight
  tft.PWM1out(255);

  // With hardware accelleration this is instant
  tft.fillScreen(RA8875_BLACK);
  tft.touchEnable(true);

	//ts.begin();
    //clearScreen(RA8875_BLACK);
	//tft.setRotation(1);
}

/*
 * clearScreen(int color) - clear screen with specified color
 */
void clearScreen(int color){
    tft.fillScreen(color);
}

void MainScreenInit(void){
  clearScreen(RA8875_BLACK);
  title.draw();
  version.draw();
  //settings.draw();
  //ecg.draw();
  //spo2.draw();
}


void SettingsScreenInit(void){
  clearScreen(RA8875_BLACK);
  confirm_button.draw();
  default_button.draw();
  cancel_button.draw();
}

void fixCoordinates(int* x, int* y){
  int temp = *x;
  *x = *y;
  *y = temp;
}

/*
TS_Point getFixedCoordinates(void){
   if (tft.bufferEmpty()) { return TS_Point(tft.s_height(), tft.s_width(), 0); }
   TS_Point p = tft.touchRead(&tx,&ty);
   while(tft.touched()){
       p = tft.touchRead(&tx,&ty); 
   }
   // Scale from 0-4000 to tft.s_width() using calibration numbers
   p.x = tft.s_height() - map(p.x, TS_MINX, TS_MAXX, 0, tft.s_height());
   p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.s_width());
   int temp = p.x;
   p.x = p.y;
   p.y = temp;
   return p;
}
*/

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
  clearScreen(RA8875_BLACK);
  //tft.drawLine(50,100,50,300,RA8875_WHITE);
  //tft.drawLine(1,2*vertical_scale,s_width-1,100,RA8875_LIGHTGREY);
  //ecg.read();
  //spo2.read();
  //sample_timer.begin(sampling_isr, 150);
  //sample_timer.priority(128);
}

void sampling_isr(void) {
    //ecg.read();
    //spo2.read();
	return;
}

void alarm_isr(void) {
    toggleInvert();
    return;
}

int display_count = 0;
int hr_counter = 0;

void loop(void) {
    //tft.drawPixel(100,100,RA8875_GREEN);
    //tft.drawPixel(100+vertical_scale,100+vertical_scale,RA8875_RED);
    //tft.drawPixel(100+horizontal_scale,100+horizontal_scale,RA8875_RED);
    showGrid();
    tft.drawCircle(300,100,70,RA8875_GREEN);
    tft.drawCircle(330,120,70,RA8875_RED);
    delay(100000);
}
/*
void loop(void) {
  //main screen
  //clearScreen(RA8875_CYAN);
  if (currentMode == HOMESCREEN){
    MainScreenInit();
    //stopAlarm();
    int delay_touch_detection = 10000;
    for (int i = 0; i < delay_touch_detection; i += 1) {
        if (tft.touched()){
            tft.touchRead(&tx,&ty);
        }
    }
    while (currentMode == HOMESCREEN){
  		if (display_count >= 10) {
    		//ecg.display_signal();
    		//spo2.display_signal();
    		display_count = 0;
    		if (hr_counter >= 10) {
  	  			//int hr = ecg.heart_rate();
    			//String s_hr = String(hr);
    			//hr_counter = 0;
				//tft.fillRect(tft.s_width()-45, 120, 45, 45, RA8875_BLACK);
  				//tft.setCursor(tft.s_width()-45, 120);
				//tft.setTextColor(RA8875_GREENYELLOW);
  				//tft.setTextSize(2);
  				//tft.println(s_hr);
                showGrid();
                //if ((hr < DEFAULT_ECG_MIN + biasECGMin) || (hr > DEFAULT_ECG_MAX + biasSP02Max)){
                //    if (!activeAlarm) {
                //        throwAlarm();
                //    }
                //}
                //else {
                //    if (activeAlarm) {
                //        stopAlarm(); 
                //    }
    		    //}  
  		}
        else { hr_counter += 1; }
        }
  		else { display_count += 1;} 

	  if (!tft.touched()) {
		continue;
	  } 
      // Retrieve the touch point
      tft.touchRead(&tx,&ty);
      //TS_Point p = getFixedCoordinates();
      if (settings.isTapped(tx,ty)){
        currentMode = ALARMSCREEN;
      }
    }
  }
  //alarm screen
  if (currentMode == ALARMSCREEN){
    currentMode = HOMESCREEN;
  }  
}*/
  
