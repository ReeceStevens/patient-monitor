#include <Adafruit_ILI9341.h>
#include <Arduino.h>
#include "interface.h"

// Constructor
Button::Button(int coord_x, int coord_y, int len, int width, int color, bool visible, Adafruit_ILI9341* tft):coord_x(coord_x), coord_y(coord_y), len(len), width(width), color(color), visible(visible), tft_interface(tft) {}

void Button::draw(){
	tft_interface->fillRect(coord_x,coord_y,len,width,color);
}

bool Button::isTapped(int x, int y){
	// Weirdness is happening right now where x and y axes are switched
	// Once this is fixed in the main script, fix this function too!
	if ((x >= coord_x) & (x <= (coord_x + width))){
		if ((y >= coord_y) & (y <= (coord_y + len))){
				return true;
		}
	}
	return false;	
}

ECGReadout::ECGReadout(int coord_x, int coord_y, int width, int len, int pin, int reset_timer, Adafruit_ILI9341* tft):coord_x(coord_x), coord_y(coord_y), len(len), width(width), pin(pin),reset_timer(reset_timer), tft_interface(tft) {
	// Allocate space for one integer per pixel length-wise
	databuffer = (int*) malloc(width * sizeof(int));		   
	current_timer = 0;
	buffer_contents = 0;
	scaling_factor = len / 500;

}

void ECGReadout::destroy(){
	free(databuffer);
}

void ECGReadout::draw(){
	tft_interface->drawRect(coord_x,coord_y,len,width, ILI9341_BLACK);
}


// Read the ECG data from the analog pin
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
	input_num =  input_num * len;
    input_num = input_num / 900;
	// shift all buffer contents down one
	for (int i = buffer_contents; i > 0; i--){
		tft_interface->drawPixel(coord_x + i, coord_y + databuffer[i], ILI9341_BLACK);
		databuffer[i] = databuffer[i-1];
		tft_interface->drawPixel(coord_x + i, coord_y + databuffer[i], ILI9341_WHITE);
	}
	tft_interface->drawPixel(coord_x, coord_y + databuffer[0], ILI9341_BLACK);
	databuffer[0] = (int) input_num;
	tft_interface->drawPixel(coord_x, coord_y + databuffer[0], ILI9341_WHITE);
	return;
}
