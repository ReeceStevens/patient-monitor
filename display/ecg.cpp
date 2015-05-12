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

volatile int avg_count = 0;
// Constructor
ECGReadout::ECGReadout(int coord_x, int coord_y, int width, int len, int pin, int reset_timer, Adafruit_ILI9341* tft):coord_x(coord_x), coord_y(coord_y), len(len), width(width), pin(pin),reset_timer(reset_timer), tft_interface(tft) {
	// Allocate space for one integer per pixel length-wise	  a
    fifo_multiplier = 9;
    fifo_size = width * fifo_multiplier; 
    fifo = Vector<double> (fifo_size);
	averager_queue = Vector<double>(5);
    //this->fifo = fifo;
    display_fifo = Vector<double> (width);
    fifo_next = 0;
    fifo_end = 1;
    disp_start = fifo_next;
    disp_end = fifo_end;
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
	//tft_interface->fillRect(60,0,tft_interface->width()-60,tft_interface->height(),ILI9341_BLUE);
	if (avg_count < 5) {
		double input_num = (double) analogRead(pin);
		double adjusted_num =  input_num * len;
    	adjusted_num = (adjusted_num / 1023);
    	// Put number in next location in fifo
		averager_queue[avg_count] = adjusted_num;
		avg_count += 1;
    	//int result = fifo.set(fifo_next, adjusted_num);
	} else {
		avg_count = 0;
		double avg;
		for (uint32_t k = 0; k < 5; k += 1) {
			avg += averager_queue[k];
		}
		avg /= 5;
    	int result = fifo.set(fifo_next, avg);
    	fifo_next = fifo_end;
    	fifo_end = (fifo_end + 1) % fifo_size;
	}

    // Move our trackers
	//tft_interface->fillRect(60,0,tft_interface->width()-60,tft_interface->height(),ILI9341_CYAN);
}

/*
 * display_signal() - clear previous signal and print existing signal onto display
 *
 */
void ECGReadout::display_signal(){
    cli(); // Disable all interrupts
	//tft_interface->fillRect(60,0,tft_interface->width()-60,tft_interface->height(),ILI9341_GREEN);
    // Trackers will always round down. Not ideal, but lets us shrink fifo size without much fuss.
	//nointerrupts();
    int newest = fifo_next/fifo_multiplier;
    int oldest = fifo_end/fifo_multiplier;
    // Make our copy of the data so we can draw while the analog pin
    // keeps sampling and updating the buffer.
    Vector<double> new_input_data(fifo);
    sei(); // Re-enable all interrupts
	//interrupts();
    Vector<double> new_display_data(width);
    for (uint32_t i = 0; i < fifo_size; i += fifo_multiplier) {
        double maximum = 0;
        for (uint32_t k = 0; k < fifo_multiplier; k += 1) {
            if (new_input_data[i+k] > maximum) {
               maximum = new_input_data[i+k];
            }
        }
        new_display_data[i/fifo_multiplier] = maximum; 
    }
	//tft_interface->fillRect(60,0,tft_interface->width()-60,tft_interface->height(),ILI9341_WHITE);
    int i = 0;
    int line_thresh = 10;
    // Draw over old data in black and new data in white
    while ((((i + oldest + 1) % width) != newest)){
        int k = (i + disp_end) % width; // Numerical position in old fifo vector (i is pixel location on screen)
        int prev = (i + disp_end + 1) % width; // Position of data point to be erased on display  
        int new_k = (i + oldest) % width; // Numerical position in new fifo vector (i is pixel location on screen)
        int new_prev = (i + oldest + 1) % width; // Position of data point to be drawn on display  
        /********* ERASING *********/ 
        if ((display_fifo[k] - display_fifo[prev]) > line_thresh){
            tft_interface->drawFastVLine(coord_x + i, (coord_y + len - display_fifo[k]), (display_fifo[k] - display_fifo[prev]), ILI9341_BLACK);
            //tft_interface->drawFastVLine(coord_x + i, (coord_y + display_fifo[k]), (display_fifo[k] - display_fifo[prev]), ILI9341_BLACK);
        }
        else if ((display_fifo[prev] - display_fifo[k]) > line_thresh){
            tft_interface->drawFastVLine(coord_x + i, (coord_y + len - display_fifo[prev]), (display_fifo[prev] - display_fifo[k]), ILI9341_BLACK);
            //tft_interface->drawFastVLine(coord_x + i, (coord_y + display_fifo[prev]), (display_fifo[prev] - display_fifo[k]), ILI9341_BLACK);
        } else { 
            // If not necessary, just color in the pixel
		    tft_interface->drawPixel(coord_x + i, (coord_y + len - display_fifo[k]), ILI9341_BLACK);
		    //tft_interface->drawPixel(coord_x + i, (coord_y + display_fifo[k]), ILI9341_BLACK);
        } 
        /********* DRAWING *********/ 
        if ((new_display_data[new_k] - new_display_data[new_prev]) > line_thresh){
            tft_interface->drawFastVLine(coord_x + i, (coord_y + len - new_display_data[new_k]), (new_display_data[new_k] - new_display_data[new_prev]), ILI9341_WHITE);
            //tft_interface->drawFastVLine(coord_x + i, (coord_y + new_display_data[new_k]), (new_display_data[new_k] - new_display_data[new_prev]), ILI9341_WHITE);
        }
        else if ((new_display_data[new_prev] - new_display_data[new_k]) > line_thresh){
            tft_interface->drawFastVLine(coord_x + i, (coord_y + len - new_display_data[new_prev]), (new_display_data[new_prev] - new_display_data[new_k]), ILI9341_WHITE);
            //tft_interface->drawFastVLine(coord_x + i, (coord_y + new_display_data[new_prev]), (new_display_data[new_prev] - new_display_data[new_k]), ILI9341_WHITE);
        } else {
		    tft_interface->drawPixel(coord_x + i, (coord_y + len - new_display_data[new_k]), ILI9341_WHITE);
        }
		//tft_interface->drawPixel(coord_x + i, (coord_y + new_display_data[new_k]), ILI9341_WHITE);

        i += 1;
    }
    // Catch the last pixel
	tft_interface->drawPixel(coord_x + i, (coord_y + len - display_fifo[(i+1)%width]), ILI9341_BLACK);
    // Store our new display information for next time
    display_fifo = new_display_data;
    disp_start = newest;
    disp_end = oldest;
	//tft_interface->fillRect(60,0,tft_interface->width()-60,tft_interface->height(),ILI9341_WHITE);
}

/*
 * ECGReadout::heart_rate() -- determine period of input signal
 * 
 * Measure interval of "silence" between waves?
 */
int ECGReadout::heart_rate() {
    double sampling_period = 0.00827; // time between samples (in seconds)
    int threshold = 50;
	int wait = 15;
	int start = -1;
    int mid = -1;
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
		if (wait < 15) {
			wait += 1;
			continue;
		}

		// Find the next peak
		if ((display_fifo[i] > threshold) && (mid == -1)){
			mid = i;
			wait = 0;
			break;
		}

		if (wait < 15) {
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
    double heart_rate1;
    double heart_rate2;
    double heart_rate3;
    if ((mid != -1) && (start != -1)) {
        heart_rate1 = 60 / ((mid - start) * sampling_period);
    } else { return 0; }
    if (finish != -1) {
        heart_rate2 = 60 / ((finish - mid) * sampling_period);
        heart_rate3 = 120 / ((finish - start) * sampling_period);
    } else {return heart_rate1;}
    double maximum = 0;
    if ((heart_rate1 >= heart_rate2) && (heart_rate1 >= heart_rate3)) {return heart_rate1; }
    if ((heart_rate2 >= heart_rate1) && (heart_rate2 >= heart_rate3)) {return heart_rate2; }
    return heart_rate3;
}

