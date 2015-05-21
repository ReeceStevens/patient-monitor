#include <Adafruit_ILI9341.h>
#include "Vector.h"

class ECGReadout {
	private:
			int coord_x; 
			int coord_y;
			int len;
			int width;
            int fifo_size;
            int fifo_multiplier;
			int pin;
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
			ECGReadout(int, int, int, int, int, int, Adafruit_ILI9341*);
			void draw(void);
			void read(void);
            void display_signal(void);
            int heart_rate(void);

};
