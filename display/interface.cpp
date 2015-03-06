#include <Adafruit_ILI9341.h>
#include "interface.h"

class button {
	public:
		int coord_x;
		int coord_y;
		int length;
		int width;
		int color;
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
