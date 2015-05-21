#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "Vector.h"
#include "spO2.h"
// To determine heart rate, we need to know how fast 
// our system is sampling. 

volatile int avg_count = 0;
volatile double max_IR = 0;
volatile double min_IR = 0;
volatile double max_R = 0;
volatile double min_R = 0;
// Constructor
spO2Readout::spO2Readout(int coord_x, int coord_y, int width, int len, int pin, int reset_timer, Adafruit_ILI9341* tft):coord_x(coord_x), coord_y(coord_y), len(len), width(width), pin_detector(pin_detector), pin_IR(pin_IR), pin_R(pin_R), reset_timer(reset_timer), tft_interface(tft) {
    // Allocate space for one integer per pixel length-wise   a
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

void spO2Readout::draw(){
    tft_interface->drawRect(coord_x,coord_y,len,width, ILI9341_BLACK);
	// Turn on LEDs for SpO2 probe
	pinmode(pin_IR, OUTPUT);
	pinmode(pin_R, OUTPUT);
	digitalWrite(pin_IR, HIGH);
	digitalWrite(pin_R, HIGH);
}

void checkMax(int check_IR,int check_R){
    if(max_IR == 0 && max_R == 0){ // only use if we dont know max and min values
        max_IR = check_IR;
        min_IR = check_IR;
        max_R = check_R;
        min_R = check_R;
        return;
    }

    if(check_IR > max_IR){
        max_IR = check_IR;
    }
    if (check_IR < min_IR){
        min_IR = check_IR;
    }
    if(check_R > max_R){
        max_R = check_R;
    }
    if (check_R < min_R){
        min_R = check_R;
    }
}

double calculateSPO2(double vmax_red, double vmax_ir, double vmin_red, double vmin_ir){
    double R = (vmax_red - vmin_red)*vmin_ir;
    double spo2 = (10.0002 * (R*R*R)) - (52.887*(R*R)) + 98.282   // will probably need to adjust these numbers
    return spo2;
}

void spO2Readout::read(){
    //tft_interface->fillRect(60,0,tft_interface->width()-60,tft_interface->height(),ILI9341_BLUE);
    if (avg_count < 5) {
        double input = (double) analogRead(pin_detector);
        double adjusted = (input /1023);
        // Put number in next location in fifo
        averager_queue[avg_count] = adjusted;
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


}

void spO2eadout::display_signal(){
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
