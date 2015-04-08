/*
 * ecg.cpp
 * Author: Reece Stevens
 * Start Date: 4.6.15
 *
 * The monitoring functions of the ECG probe for the 
 * TEWH Patient Monitor. File still under construction.
 *
 * Function goals:
 *   a. Detect the heart rate of the input signal
 *   b. Detect abnormalities or changes in signal over time
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "ecg.h"
// To determine heart rate, we need to know how fast 
// our system is sampling. 

// Constructor
ECGReadout::ECGReadout(int coord_x, int coord_y, int width, int len, int pin, int reset_timer, Adafruit_ILI9341* tft):coord_x(coord_x), coord_y(coord_y), len(len), width(width), pin(pin),reset_timer(reset_timer), tft_interface(tft) {
	// Allocate space for one integer per pixel length-wise
	databuffer = (int*) malloc(width * sizeof(int));		   
	input_buffer = (int*) malloc(width * sizeof(int));		   
	diff_buffer = (int*) malloc((width-1) * sizeof(int));		   
	current_timer = 0;
	buffer_contents = 0;
	scaling_factor = len / 500;

}

void ECGReadout::destroy(){
	free(databuffer);
}

// Destructor for the compiler
ECGReadout::~ECGReadout() {
    destroy();
}

void ECGReadout::draw(){
	tft_interface->drawRect(coord_x,coord_y,len,width, ILI9341_BLACK);
}

/*
 * ECGReadout::read() -- Read ECG signal from analog pin
 *
 */
void ECGReadout::read(){
       if (buffer_contents < width - 1){
		buffer_contents ++;
	} 
	// Reset timer if time is up
	current_timer = 0;
	// Analog input is scaled from 0 to 1023
	// with a 5V max
	double input_num = (double) analogRead(15);
	double adjusted_num =  input_num * len;
    adjusted_num = (adjusted_num / 1023);
	// shift all buffer contents down one
        int line_thresh = 40;
	for (int i = buffer_contents; i > 1; i--){
  
                if ((databuffer[i] - databuffer[i-1]) > line_thresh){
                  tft_interface->drawFastVLine(coord_x + i, (coord_y + len - databuffer[i]), (databuffer[i] - databuffer[i-1]), ILI9341_BLACK);
                }
                if ((databuffer[i-1] - databuffer[i]) > line_thresh){
                  tft_interface->drawFastVLine(coord_x + i, (coord_y + len - databuffer[i-1]), (databuffer[i-1] - databuffer[i]), ILI9341_BLACK);
                }
                
		tft_interface->drawPixel(coord_x + i, (coord_y + len - databuffer[i]), ILI9341_BLACK);
		databuffer[i] = databuffer[i-1];
        input_buffer[i] = input_buffer[i-1];
        
                if ((databuffer[i-1] - databuffer[i-2]) > line_thresh){
                  tft_interface->drawFastVLine(coord_x + i, (coord_y + len - databuffer[i-1]), (databuffer[i-1] - databuffer[i-2]), ILI9341_WHITE);
                }
                if ((databuffer[i-2] - databuffer[i-1]) > line_thresh){
                  tft_interface->drawFastVLine(coord_x + i, (coord_y + len - databuffer[i-2]), (databuffer[i-2] - databuffer[i-1]), ILI9341_WHITE);
                }
                /*if (abs(databuffer[i-1] - databuffer[i-2]) > 25){
		  tft_interface->drawFastVLine(coord_x + i, (coord_y + len - databuffer[i]), (databuffer[i-1] - databuffer[i-2]), ILI9341_WHITE);
                }*/
                
		tft_interface->drawPixel(coord_x + i, (coord_y + len - databuffer[i]), ILI9341_WHITE);
	}
	//tft_interface->drawFastVLine(coord_x + 1, (coord_y + len - databuffer[1]), (databuffer[1] - databuffer[0]), ILI9341_BLACK);
	tft_interface->drawPixel(coord_x, (len + coord_y - databuffer[0]), ILI9341_BLACK);
	databuffer[1] =  databuffer[0];	
	databuffer[0] = (int) adjusted_num;
	//tft_interface->drawFastVLine(coord_x + 1, (coord_y + len - databuffer[1]), (databuffer[1] - databuffer[0]), ILI9341_WHITE);
	//databuffer[0] = (int) adjusted_num;
    input_buffer[0] = (int) input_num;
	tft_interface->drawPixel(coord_x, (len + coord_y - databuffer[0]), ILI9341_WHITE);
	return;
}

/*
 * ECGReadout::heart_rate() -- determine period of input signal
 * 
 * Measure interval of "silence" between waves?
 */
int ECGReadout::heart_rate() {
    double sampling_period = 0.033333; // time between samples (in seconds)
    int threshold = 90;
	int wait = 10;
	int start = -1;
        int finish = -1;
    // Calcluate the finite difference approximation
    for (int i = 0; i < (width - 1); i += 1){
		// Find the first peak
		if ((databuffer[i] > threshold) && (start == -1)){
			start = i;
			wait = 0;
		}
		// Delay after we find the peak 
		// so we don't detect another sample on the same peak
		if (wait < 4) {
			wait += 1;
			continue;
		}
		// Find the next peak
		if ((databuffer[i] > threshold) && (finish == -1)){
			finish = i;
			wait = 0;
			break;
		}
	}
    double heart_rate = 60 / ((finish - start) * sampling_period);
    return heart_rate;
}

/*
 * ECGReadout::heart_rate() -- determine period of input signal
 * 
 * Measure interval of "silence" between waves?
 */
/*
int ECGReadout::heart_rate() {
    double sampling_period = 42; // time between samples (in seconds)
    int threshold = 42;
    int start = -1; 
    int down = 1;
    int finish = -1;
    // Calcluate the finite difference approximation
    for (int i = 0; i < (width - 1); i += 1){
        diff_buffer[i] = input_buffer[i+1] - input_buffer[i];
        // Find our start when the difference is small (silence)
        if (down) {
            if (start == -1){
                if (abs(diff_buffer[i]) < threshold) {
                    start = i;
                }
            } 
            // Find our end when the difference increases (signal)
            if (abs(diff_buffer[i]) > threshold) {
                down = 0;
            }
        }
        // Find when silence starts again (a complete period)
        else { 
            if (abs(diff_buffer[i]) < threshold) {
                finish = i;
            }
        } 
    } 
    // Once period is calculated, return heart rate in bpm.
    double heart_rate = 60 / ((finish - start) * sampling_period);
    return heart_rate;
} */
