#ifndef __CircleFifo_h__
#define __CircleFifo_h__ 1

#include "Vector.h"

template <typename T>

class CircleFifo {
private:
	int length;
	T* data;
	int front;
	int end;
	int mod(int a, int b) {
		if (b > 0){
			if (a < b) { return a; }
			else {
				do {
					a = a - b;
				} while (a > b);
			return a;
		} 
		else { return -1; } // Error state. Mod will not be called in this context.
	}
	
	~CircleFifo() {
		delete [] data;
	}

public:
	// Requests length of N+1 for circular operation
	CircleFifo(int N) {
		length = N+1;
		data = new T[length];	
		next = 0;
		end = 1;
	}

	void add(const T& payload) {
		data[next] = T;
		next = end;
		end = mod((end + 1), length);
	}
	
	T& operator[](const int k) {
		if (length <= k) {
          int* p = 0;
          p = (int*) 1; // Intentionally crash.
		}	
		return data[mod(next+k, length)];
	}
};

#endif
