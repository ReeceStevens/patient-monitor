/*
 * ecg.h
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
//#include "Adafruit_RA8875.h"
#include "Vector.h"
//#include <Adafruit_ILI9341.h>

volatile int avg_count = 0;

class ECGReadout: public ScreenElement {
private:
    int fifo_size;
    int fifo_multiplier;
	int pin;
	int reset_timer;
	int current_timer;
	double scaling_factor;
	int buffer_contents;
    volatile Vector<double> fifo;
	volatile Vector<double> averager_queue;
    Vector<double> display_fifo;
    volatile uint32_t fifo_next;
    volatile int fifo_end;
    int disp_start; 
    int disp_end;
    int trace_color;
    int background_color;

public:
    ECGReadout(int row, int column, int len, int width, int pin, int reset_timer, int trace_color, int background_color, Adafruit_RA8875* tft):ScreenElement(row,column,len,width,tft), pin(pin),reset_timer(reset_timer), trace_color(trace_color), background_color(background_color) {
	    // Allocate space for one integer per pixel length-wise	  a
        fifo_multiplier = 4;
        fifo_size = real_width * fifo_multiplier; 
        fifo = Vector<double> (fifo_size);
	    averager_queue = Vector<double>(5);
        //this->fifo = fifo;
        display_fifo = Vector<double> (real_width);
        fifo_next = 0;
        fifo_end = 1;
        disp_start = fifo_next;
        disp_end = fifo_end;
	    current_timer = 0;
	    buffer_contents = 0;
	    scaling_factor = real_len / 500;
    };
    void draw_border(void){
	    tft_interface->drawRect(coord_x,coord_y,real_width,real_len, trace_color);
    };
    void draw(void){
        tft_interface->fillRect(coord_x,coord_y,real_width,real_len,background_color);
    };
	void read(void){
	    if (avg_count < 5) {
		    double input_num = (double) analogRead(pin);
		    double adjusted_num =  input_num * real_len;
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

    };
    void display_signal(void){
        draw_border(); 
        cli(); // Disable all interrupts

        // Trackers will always round down. Not ideal, but lets us shrink fifo size without much fuss.
        int newest = fifo_next/fifo_multiplier;
        int oldest = fifo_end/fifo_multiplier;
        // Make our copy of the data so we can draw while the analog pin
        // keeps sampling and updating the buffer.
        //Vector<double> new_input_data(fifo);
        Vector<double> new_display_data(real_width);
        for (uint32_t i = 0; i < fifo_size; i += fifo_multiplier) {
            double maximum = 0;
            for (uint32_t k = 0; k < fifo_multiplier; k += 1) {
                if (fifo[i+k] > maximum) {
                 maximum = fifo[i+k];
                }
            }
            new_display_data[i/fifo_multiplier] = maximum; 
        }
        sei(); // Re-enable all interrupts
        int i = 0;
        int line_thresh = 10;
        // Draw over old data in black and new data in white
        while ((((i + oldest + 1) % real_width) != newest)){
            int k = (i + disp_end) % real_width; // Numerical position in old fifo vector (i is pixel location on screen)
            int prev = (i + disp_end + 1) % real_width; // Position of data point to be erased on display  
            int new_k = (i + oldest) % real_width; // Numerical position in new fifo vector (i is pixel location on screen)
            int new_prev = (i + oldest + 1) % real_width; // Position of data point to be drawn on display  
            /********* ERASING *********/ 
            if ((display_fifo[k] - display_fifo[prev]) > line_thresh){
                tft_interface->drawFastVLine(coord_x + i, (coord_y + real_len - display_fifo[k]), (display_fifo[k] - display_fifo[prev]), background_color);
                //tft_interface->drawFastVLine(coord_x + i, (coord_y + display_fifo[k]), (display_fifo[k] - display_fifo[prev]), background_color);
            }
            else if ((display_fifo[prev] - display_fifo[k]) > line_thresh){
                tft_interface->drawFastVLine(coord_x + i, (coord_y + real_len - display_fifo[prev]), (display_fifo[prev] - display_fifo[k]), background_color);
                //tft_interface->drawFastVLine(coord_x + i, (coord_y + display_fifo[prev]), (display_fifo[prev] - display_fifo[k]), background_color);
            } else { 
                 // If not necessary, just color in the pixel
		        tft_interface->drawPixel(coord_x + i, (coord_y + real_len - display_fifo[k]), background_color);
		    //tft_interface->drawPixel(coord_x + i, (coord_y + display_fifo[k]), background_color);
            } 
            /********* DRAWING *********/ 
            if ((new_display_data[new_k] - new_display_data[new_prev]) > line_thresh){
                tft_interface->drawFastVLine(coord_x + i, (coord_y + real_len - new_display_data[new_k]), (new_display_data[new_k] - new_display_data[new_prev]), trace_color);
                //tft_interface->drawFastVLine(coord_x + i, (coord_y + new_display_data[new_k]), (new_display_data[new_k] - new_display_data[new_prev]), trace_color);
            }
            else if ((new_display_data[new_prev] - new_display_data[new_k]) > line_thresh){
                tft_interface->drawFastVLine(coord_x + i, (coord_y + real_len - new_display_data[new_prev]), (new_display_data[new_prev] - new_display_data[new_k]), trace_color);
                //tft_interface->drawFastVLine(coord_x + i, (coord_y + new_display_data[new_prev]), (new_display_data[new_prev] - new_display_data[new_k]), trace_color);
            } else {
		        tft_interface->drawPixel(coord_x + i, (coord_y + real_len - new_display_data[new_k]), trace_color);
            }
		    //tft_interface->drawPixel(coord_x + i, (coord_y + new_display_data[new_k]), trace_color);

            i += 1;
        }
        // Catch the last pixel
	    tft_interface->drawPixel(coord_x + i, (coord_y + real_len - display_fifo[(i+1)%real_width]), background_color);
        // Store our new display information for next time
        display_fifo = new_display_data;
        disp_start = newest;
        disp_end = oldest;
	    //tft_interface->fillRect(60,0,tft_interface->real_width()-60,tft_interface->height(),trace_color);
    };
    int heart_rate(void){
        double sampling_period = 0.00827; // time between samples (in seconds)
        int threshold = 40;
	    int wait = 15;
	    int start = -1;
        int mid = -1;
        int finish = -1;
        // Calcluate the finite difference approximation
        for (int i = 0; i < (real_width - 1); i += 1){
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
    };
};
