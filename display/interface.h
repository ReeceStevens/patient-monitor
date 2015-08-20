#include <Adafruit_ILI9341.h>
#include <Arduino.h>

class ScreenElement {
private:
	int coord_x;
	int coord_y;
	int len;
	int width;
    int row;
    int column;
    int real_len;
    int real_width;
    Adafruit_ILI9341* tft_interface;

public:
    ScreenElement(int row, int column, int len, int width, Adafruit_ILI9341* tft_interface):row(row),column(column),len(len),width(width),tft_interface(tft_interface){
        coord_x =  vertical_scale*(row-1);
        coord_y = horizontal_scale*(column-1);
        real_len = vertical_scale*len;
        real_width = horizontal_scale*width;
    }
}



/*
 * TODO: Write a function that uses the button_str argument
 * and creates a button label. Center the label based on the size
 * of the button.
 */
class Button : public ScreenElement{
private:
    int color;
    char* button_str;
    Adafruit_ILI9341* tft_interface;

public:
    bool visible;
    bool lastTapped;

    Button(int row, int column, int len, int width, int color, bool visible, char* button_str, Adafruit_ILI9341* tft):ScreenElement(row,column,len,width,tft_interface), color(color), visible(visible), button_str(button_str) { 
        lastTapped = 0;
    };

    void draw(void){
	    tft_interface->fillRect(coord_x,coord_y,real_len,real_width,color);
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

class TextBox : public ScreenElement {
private:
    int background_color;
    int text_color;
    char* str;

public:
    bool visible;

    TextBox(int row, int column, int len, int width, int background_color, int text_color, bool visible, char* str, Adafruit_ILI9341* tft):ScreenElement(row,column,len,width,tft_interface), background_color(background_color), text_color(text_color), str(str), visible(visible) { };


}

