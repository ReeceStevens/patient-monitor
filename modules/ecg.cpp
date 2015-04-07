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
void ~ECGReadout {
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
	// If it isn't time, just exit
	if (current_timer < reset_timer){
		current_timer ++;	
		return;
	}
	if (buffer_contents < width - 1){
		buffer_contents ++;
	}
	// Reset timer if time is up
	current_timer = 0;
	// Analog input is scaled from 0 to 1023
	// with a 5V max
	double input_num = (double) analogRead(15);
	double adjusted_num =  input_num * len;
    adjusted_num = adjusted_num / 900;
	// shift all buffer contents down one
	for (int i = buffer_contents; i > 0; i--){
		tft_interface->drawPixel(coord_x + i, coord_y + databuffer[i], ILI9341_BLACK);
		databuffer[i] = databuffer[i-1];
        input_buffer[i] = input_buffer[i-1];
		tft_interface->drawPixel(coord_x + i, coord_y + databuffer[i], ILI9341_WHITE);
	}
	tft_interface->drawPixel(coord_x, coord_y + databuffer[0], ILI9341_BLACK);
	databuffer[0] = (int) adjusted_num;
    input_buffer[0] = (int) input_num;
	tft_interface->drawPixel(coord_x, coord_y + databuffer[0], ILI9341_WHITE);
	return;
}

/*
 * ECGReadout::heart_rate() -- determine period of input signal
 * 
 * Measure interval of "silence" between waves?
 */
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
}

/*
 * ECGReadout::check_okay(void) - check for waveform abnormalities
 *
 */
bool ECGReadout::check_okay(void) {
    int hr_threshold;
      
}


