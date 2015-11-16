/*
 * stats.h
 *
 * Provides helper functions for common calculations such as avg, std, etc.
 *
 * Operates on Vector type.
 */

#include <math.h>

#include "Vector.h"

class Stats {
public:
	static double avg (Vector<double> a) {
		double avg = 0;
		for (int i = 0; i < a.size(); i += 1) {
			avg += a[i];		
		}
		avg /= a.size();
		return avg;
	}

	static double std (Vector<double> a) {
		double a_avg = avg(a);
		double std = 0;
		for (int i = 0; i < a.size(); i += 1) {
			double diff = a[i] - avg;
			std += diff*diff;	
		}
		std /= a.size();
		std = sqrt(std);
		return std;
	}
}
