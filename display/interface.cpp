#include <Adafruit_ILI9341.h>
#include "interface.h"

// Constructor
button::button(int in_coord_x, int in_coord_y, int in_len, int in_width, int in_color, bool in_visible) {
	coord_x = in_coord_x;
	coord_y = in_coord_y;
	len = in_len;
	width = in_width;
	color = in_color;
	visible = in_visible;
        lastTapped = false;
}

void button::draw(Adafruit_ILI9341 tft){
	tft.fillRect(coord_x,coord_y,len,width,color);
}

bool button::isTapped(int x, int y){
	// Weirdness is happening right now where x and y axes are switched
	// Once this is fixed in the main script, fix this function too!
	if ((x >= coord_x) & (x <= (coord_x + width))){
		if ((y >= coord_y) & (y <= (coord_y + len))){
				return true;
		}
	}
	return false;	
}
