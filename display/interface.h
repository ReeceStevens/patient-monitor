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

class ECGReadout {
	private:
			int coord_x; 
			int coord_y;
			int len;
			int width;
			int pin;
			int* databuffer;
			int reset_timer;
			int current_timer;
			double scaling_factor;
			int buffer_contents;
			Adafruit_ILI9341* tft_interface;

	public:
			void destroy(void);
			void draw(void);
			void read(void);
			ECGReadout(int, int, int, int, int, int, Adafruit_ILI9341*);

};

