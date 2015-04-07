#include <Adafruit_ILI9341.h>
#ifndef _ecg_h
#define _ecg_h

class ECGReadout {
	private:
			int coord_x; 
			int coord_y;
			int len;
			int width;
			int pin;
			int* databuffer;
            int* input_buffer;
            int* diff_buffer;
			int reset_timer;
			int current_timer;
			double scaling_factor;
			int buffer_contents;
			Adafruit_ILI9341* tft_interface;

			void destroy(void);
            void ~ECGReadout(void);

	public:
			ECGReadout(int, int, int, int, int, int, Adafruit_ILI9341*);
			void draw(void);
			void read(void);
            uint32_t heart_rate(void);

};

#endif
