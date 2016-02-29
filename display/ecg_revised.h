#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "CircleFifo.h"

class ECGReadout : public ScreenElement {
private:
	int fifo_size;
	int avg_size;
	int avg_cursor;
	int display_cursor;
	int pin;
	CircleFifo<int> fifo;
	int scaling factor;
	Vector<int> avg_queue;
	int trace_color;
	int background_color;

public:	
    ECGReadout(int row, int column, int len, int width, int pin, 
				int trace_color, int background_color, Adafruit_RA8875* tft):
				ScreenElement(row,column,len,width,tft), pin(pin), trace_color(trace_color),
			   	background_color(background_color) {
		fifo_size = real_width;
		fifo = CircleFifo<int>(fifo_size);
		avg_size = 5;
		avg_cursor = 0;
		display_cursor = 0;
		avg_queue = Vector<int>(avg_size);
		scaling_factor = real_len;			
	}
    void draw_border(void){
	    tft_interface->drawRect(coord_x,coord_y,real_width,real_len,trace_color);
    };
    void draw(void){
        tft_interface->fillRect(coord_x,coord_y,real_width,real_len,background_color);
		draw_border();
    };

	void read(void) {
		if (avg_cursor < avg_size) {
			int input = analogRead(pin);
			avg_queue[avg_count] = input;
			avg_cursor += 1;
		} else {
			avg_cursor = 0;
			int avg;
			for (int i = 0; i < avg_size; i ++ ) {
				avg += avg_queue[i];
			}
			avg /= avg_size;
			fifo.add(avg);
		}	
	}

	void display_signal(void) {
		// Get newest value from fifo
		cli();
		int new_val = fifo[0];
		int last_val = fifo[1];
		sei();
		int threshold = 10;
		int display = new_val * real_len;
		display /= 1023;
		tft->drawFastVLine(coord_x + display_cursor, coord_y, coord_y + real_len, background_color);
		if ((new_val - last_val > threshold) || (new_val - last_val < -threshold)) {
			tft->drawFastVLine(coord_x + display_cursor, coord_y+last_val, coord_y + new_val, trace_color);
		} else {
			tft->drawPixel(coord_x + display_cursor, coord_y + display, trace_color);
		}
		display_cursor = mod(display_cursor+1, real_width); // Advance display cursor
		tft->drawFastVLine(coord_x + display_cursor, coord_y, coord_y + real_width, RA8875_WHITE); // Draw display cursor
	}

};
