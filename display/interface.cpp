#include <Adafruit_ILI9341.h>
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
