#include <Adafruit_ILI9341.h>
#include "interface.h"

// Constructor
button::button(int coord_x, int coord_y, int length, int width, int color, bool visible) {
	_coord_x = coord_x;
	_coord_y = coord_y;
	_length = length;
	_width = width;
	_color = color;
	_visible = visible;
}

void button::draw(Adafruit_ILI9341 tft){
	tft.fillRect(coord_x,coord_y,length,width,color);
}

bool button::isTapped(int x, int y){
	// Weirdness is happening right now where x and y axes are switched
	// Once this is fixed in the main script, fix this function too!
	if ((x >= coord_x) & (x <= (coord_x + width))){
		if ((y >= coord_y) & (y <= (coord_y + length))){
				return true;
		}
	}
	return false;	
}
