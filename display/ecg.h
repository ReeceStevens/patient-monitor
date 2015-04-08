#include <Adafruit_ILI9341.h>

class ECGReadout {
	private:
			int coord_x; 
			int coord_y;
			int len;
			int width;
			int pin;
			int* databuffer;
			int* diff_buffer;
			int* input_buffer;
			int reset_timer;
			int current_timer;
			double scaling_factor;
			int buffer_contents;
			Adafruit_ILI9341* tft_interface;

			void destroy(void);

	public:
			ECGReadout(int, int, int, int, int, int, Adafruit_ILI9341*);
			void draw(void);
			void read(void);
            int heart_rate(void);
            ~ECGReadout(void);

};
