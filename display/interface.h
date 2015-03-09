#include <Adafruit_ILI9341.h>

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
			void draw(void);
			bool isTapped(int, int);
            Button(int,int,int,int,int,bool,Adafruit_ILI9341*);
};
