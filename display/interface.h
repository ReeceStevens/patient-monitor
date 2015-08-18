#include <Adafruit_ILI9341.h>
#include <Arduino.h>

// Constructor
class Button {
private:
	int coord_x;
	int coord_y;
	int len;
	int width;
    int color;
    Adafruit_ILI9341* tft_interface;

public:
    bool visible;
    bool lastTapped;

    Button(int row, int column, int len, int width, int color, bool visible, Adafruit_ILI9341* tft):row(row), column(column), len(len), width(width), color(color), visible(visible), tft_interface(tft) {
        int coord_x =  vertical_scale*(row-1);
        int coord_y = horizontal_scale*(column-1);
        int real_len = vertical_scale*len;
        int real_width = horizontal_scale*width;
    };

    void draw(void){
	    tft_interface->fillRect(vertical_scale*(row-1),horizontal_scale*(column-1),vertical_scale*len,horizontal_scale*width,color);
    }

    void isTapped(int x, int y){
	    // Weirdness is happening right now where x and y axes are switched
	    // Once this is fixed in the main script, fix this function too!
	    if ((x >= coord_x) & (x <= (coord_x + real_width))){
		    if ((y >= coord_y) & (y <= (coord_y + real_len))){
				    return true;
		    }
	    }
	    return false;	
    }
}
