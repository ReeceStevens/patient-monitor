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
 * TODO:
 *  1. Fix heart rate function to use display fifo (more stable data)
 *  2. Use system interrupts to call the read() function
 *  3. Remove the arrays from the data structure (not needed)
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "Vector.h"
#include "ecg.h"
// To determine heart rate, we need to know how fast 
// our system is sampling. 

// Constructor
ECGReadout::ECGReadout(int coord_x, int coord_y, int width, int len, int pin, int reset_timer, Adafruit_ILI9341* tft):coord_x(coord_x), coord_y(coord_y), len(len), width(width), pin(pin),reset_timer(reset_timer), tft_interface(tft) {
	// Allocate space for one integer per pixel length-wise	   
    fifo = Vector<double> ((uint32_t) width);
    //this->fifo = fifo;
    display_fifo = Vector<double> (width);
    //this->display_fifo = display_fifo;
    //fifo_next = width-1;
    fifo_next = 0;

    fifo_end = 1;
    disp_start = (fifo_next + 1) % width;
    disp_end = (fifo_next - 1) % width;
	current_timer = 0;
	buffer_contents = 0;
	scaling_factor = len / 500;

}

void ECGReadout::draw(){
	tft_interface->drawRect(coord_x,coord_y,len,width, ILI9341_BLACK);
}

/*
 * read() - read from analog pin and push data into circular fifo
 *
 */
void ECGReadout::read(){
    
	double input_num = (double) analogRead(pin);
	double adjusted_num =  input_num * len;
    adjusted_num = (adjusted_num / 1023);
    // Put number in next location in fifo
    int result = fifo.set(fifo_next, adjusted_num);
    if (!result) { tft_interface->fillRect(60,0,tft_interface->width()-60,tft_interface->height(),ILI9341_RED); }

    // Move our trackers
    fifo_next = fifo_end;
    fifo_end = (fifo_end + 1) % width;
}

/*
 * display_signal() - clear previous signal and print existing signal onto display
 *
 */
void ECGReadout::display_signal(){
    cli(); // Disable all interrupts
	tft_interface->fillRect(60,0,tft_interface->width()-60,tft_interface->height(),ILI9341_GREEN);
    int new_start = fifo_next;
    int new_end = fifo_end;
    int disp_buffer_size = buffer_contents;
    // Make our copy of the data so we can draw while the analog pin
    // keeps sampling and updating the buffer.
    Vector<double> new_display_data = fifo;
    sei(); // Re-enable all interrupts
	tft_interface->fillRect(60,0,tft_interface->width()-60,tft_interface->height(),ILI9341_WHITE);
    int i = 0;
    int line_thresh = 40;
    // Draw over old data in black and new data in white
    while ((((i + new_end) % width) != new_start) ){
        int k = (i + disp_end) % width; // Numerical position in old fifo vector (i is pixel location on screen)
		int prev;
		if (i == 0) { prev = k; }
        prev = (i + disp_end - 1) % width; // Position of data point to be erased on display  
        int new_k = (i + new_end) % width; // Numerical position in new fifo vector (i is pixel location on screen)
		int new_prev;
		if (i == 0) { new_prev = new_k; }
        new_prev = (i + new_end - 1) % width; // Position of data point to be drawn on display  
        /********* ERASING *********/ 
        if ((display_fifo[k] - display_fifo[prev]) > line_thresh){
            tft_interface->drawFastVLine(coord_x + i, (coord_y + len - display_fifo[k]), (display_fifo[k] - display_fifo[prev]), ILI9341_BLACK);
        }
        else if ((display_fifo[prev] - display_fifo[k]) > line_thresh){
            tft_interface->drawFastVLine(coord_x + i, (coord_y + len - display_fifo[prev]), (display_fifo[prev] - display_fifo[k]), ILI9341_BLACK);
        } else { 
            // If not necessary, just color in the pixel
		    tft_interface->drawPixel(coord_x + i, (coord_y + len - display_fifo[k]), ILI9341_BLACK);
        } 
        /********* DRAWING *********/ 
        if ((new_display_data[new_k] - new_display_data[new_prev]) > line_thresh){
            tft_interface->drawFastVLine(coord_x + i, (coord_y + len - new_display_data[new_k]), (new_display_data[new_k] - new_display_data[new_prev]), ILI9341_WHITE);
        }
        if ((new_display_data[new_prev] - new_display_data[new_k]) > line_thresh){
            tft_interface->drawFastVLine(coord_x + i, (coord_y + len - new_display_data[new_prev]), (new_display_data[new_prev] - new_display_data[new_k]), ILI9341_WHITE);
        }
		tft_interface->drawPixel(coord_x + i, (coord_y + len - new_display_data[new_k]), ILI9341_WHITE);

        i += 1;
    }
    // Store our new display information for next time
    display_fifo = new_display_data;
    disp_start = new_start;
    disp_end = new_end;
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
		if ((display_fifo[i] > threshold) && (start == -1)){
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
		if ((display_fifo[i] > threshold) && (finish == -1)){
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
