#include <Adafruit_ILI9341.h>
#include "Vector.h"

class spO2Readout {
	private:
			int coord_x; 
			int coord_y;
			int len;
			int width;
            int fifo_size;
            int fifo_multiplier;
			int pin_detector_in;
			int pin_detector_out;
			int pin_IR;
			int pin_R;
			int reset_timer;
			int current_timer;
			double scaling_factor;
			int buffer_contents;
            volatile Vector<double> fifo;
            volatile Vector<double> averager_queue;
            Vector<double> display_fifo;
            volatile uint32_t fifo_next;
            volatile int fifo_end;
            int disp_start; 
            int disp_end;
			Adafruit_ILI9341* tft_interface;

			void destroy(void);

	public:
			spO2Readout(int, int, int, int, int, int, int, int, int, Adafruit_ILI9341*);
			void draw(void);
			void read(void);
            void display_signal(void);
			Vector<double>& getFifo(void);
            int heart_rate(void);

};
