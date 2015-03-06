#include <Adafruit_ILI9341.h>

class button {
	public:
			int coord_x;
			int coord_y;
			int length;
			int width;
			int color;
			bool visible;
			void draw(Adafruit_ILI9341);
			bool isTapped(int, int);
};
