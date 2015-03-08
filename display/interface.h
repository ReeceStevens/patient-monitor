#include <Adafruit_ILI9341.h>

class button {
	public:
			int coord_x;
			int coord_y;
			int len;
			int width;
			int color;
			bool visible;
                        bool lastTapped;
			void draw(Adafruit_ILI9341);
			bool isTapped(int, int);
                        button(int,int,int,int,int,bool);
};
