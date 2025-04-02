#ifndef VXMATH_H
#define VXMATH_H

#include "Generics.h"
#include <vector>
#include <array>
#include <cmath>
#include <stdio.h>
#include <iostream>

#define _WRIST 0
#define INDEX 5
#define MIDDLE 9
#define RING 13

#define PITCH0LOWERBOUND 27.5
#define PITCH0UPPERBOUND 57.5
#define MINPITCH 0
#define MAXPITCH 90

// PERSON'S DATA
extern float height;
extern float I2RCONST;
extern float M2WCONST;
extern float VCC;

#define HEIGHT2ARMSPAN 3.091
#define SINGLE_ARMSPAN height / HEIGHT2ARMSPAN
#define EYE_TO_ARM_BREAKOFF 5
#define CAMERA_ARMSPAN SINGLE_ARMSPAN - EYE_TO_ARM_BREAKOFF
#define VIOX_CAMERA_INCH_OFFSET_TO_SHOULDER 4

#define UNSCRUNCH_WRIST 4.5		// 90 / 20
#define PI 3.141592653589793
#define DEG2RAD PI / 180
#define RAD2DEG 180 / PI

class Stabilizer {

public:
	Stabilizer();
	double update(double value);
	double get_value();
	
private:
	std::vector<double> acceptable_values;
	std::vector<double> detected_values;
	double acceptable_outlier;
	double detected_outlier;

};

std::array<double, 6> rotation_get(const std::vector<std::vector<int>>& points, char hand);

double interpolate(double x, double x_min, double x_max, double y_min, double y_max);
double normalize(const std::vector<int>& vec);
double distancevec(const std::vector<int>& point1, const std::vector<int>& point2);
double distancelengths(double length1, double length2);
double clamp(double value, double minval, double maxval);
double get_I2R(const std::vector<std::vector<int>>& lmks);
double get_I2R_reference_from_depth(double depth);
double get_M2W(const std::vector<std::vector<int>>& lmks);
double get_M2W_reference_from_depth(double depth);
double get_forward_distance(const std::vector<std::vector<int>>& lmks);
std::pair<double, double> get_xy_inches(const std::vector<std::vector<int>>& lmks, double depth);

double dynamic_VCC(double depth);
double px2inch(double depth, int px_length);
double horizontal_px_to_inch(double x_pixel, double depth);

double round_to_nearest_num(double num, double to);

double ROLL(const std::vector<std::vector<int>>& lmks);
double YAW(const std::vector<std::vector<int>>& unrolled, double supposed_depth, char hand);
double PITCH(const std::vector<std::vector<int>>& lmks, double supposed_depth);

#endif // VXMATH_H
