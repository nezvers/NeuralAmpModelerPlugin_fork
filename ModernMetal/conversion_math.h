#pragma once
#include <cmath>


inline double db_to_volume(double x) {
	return std::pow(10.0, 0.05 * x);
}

inline double volume_to_db(double x) {
	return 10.0 * log10(x);
}
