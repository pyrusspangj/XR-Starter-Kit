#include "VXMath.h"

float height = essentials_get("height");
float I2RCONST = 372;
float M2WCONST = 884;
float VCC = 0.01875;

const double IMAGE_WIDTH = 640.0;
const double IMAGE_CENTER_X = IMAGE_WIDTH / 2;
const double HORIZONTAL_FOV_DEGREES = 90.0;
const double DEGREE_PER_PIXEL = HORIZONTAL_FOV_DEGREES / IMAGE_WIDTH;

std::array<double, 6> rotation_get(const std::vector<std::vector<int>>& points, char hand) {
	double roll = ROLL(points);
	double depth = get_forward_distance(points);
	double yaw = YAW(points, depth, hand);
	double pitch = PITCH(points, depth);
	
	std::pair<double, double> xy_inches = get_xy_inches(points, depth);
	double x_inches = xy_inches.first;
	double y_inches = xy_inches.second;
	
	std::array<double, 6> rotations = {roll, yaw, pitch, depth, x_inches, y_inches};
	return rotations;
}

double interpolate(double x, double x_min, double x_max, double y_min, double y_max) {
	if(x_min == x_max) {
		// xmin and xmax cannot be the same value
		return 0;
	}
	
	double ratio = (x - x_min) / (x_max - x_min);
	double y = y_min + ratio * (y_max - y_min);
	
	return y;
}

double normalize(const std::vector<int>& vec) {
	double magnitude = 0.0;
	
	for(int val: vec) {
		magnitude += std::pow(val, 2);
	}
	
	magnitude = std::sqrt(magnitude);
	return magnitude;
}

double distancevec(const std::vector<int>& point1, const std::vector<int>& point2) {
	return std::sqrt(std::pow((point1[1] - point2[1]), 2) + std::pow((point1[2] - point2[2]), 2));
}

double distancelengths(double length1, double length2) {
	return std::sqrt(std::pow(length1, 2) + std::pow(length2, 2));
}

double clamp(double value, double minval, double maxval) {
	return std::max(minval, std::min(maxval, value));
}

double get_I2R(const std::vector<std::vector<int>>& lmks) {
	return distancevec(lmks[INDEX], lmks[RING]);
}

double get_I2R_reference_from_depth(double depth) {
	return I2RCONST / depth;
}

double get_M2W(const std::vector<std::vector<int>>& lmks) {
	return distancevec(lmks[MIDDLE], lmks[_WRIST]);
}

double get_M2W_reference_from_depth(double depth) {
	return M2WCONST / depth;
}

double get_forward_distance(const std::vector<std::vector<int>>& lmks) {
	double this_I2R = get_I2R(lmks);
	double this_M2W = get_M2W(lmks);
	
	double I2R_distance_hypotenuse = this_I2R != 0 ? I2RCONST / this_I2R : I2RCONST;
	double M2W_distance_hypotenuse = this_M2W != 0 ? M2WCONST / this_M2W : M2WCONST;
	
	double better_depth = std::min(I2R_distance_hypotenuse, M2W_distance_hypotenuse);
	
	if(better_depth >= CAMERA_ARMSPAN) {
		return CAMERA_ARMSPAN;
	} else {
		return better_depth;
	}
}

std::pair<double, double> get_xy_inches(const std::vector<std::vector<int>>& lmks, double depth) {
	std::vector<int> middle = lmks[MIDDLE];  // Use for general use cases
	int x_pixels = middle[1] - (capture_width / 2);
	int y_pixels = -(middle[2] - (capture_height / 2));
	
	//std::cout << "x pixels: " << x_pixels << ", middle: " << middle[1] << std::endl;
	
	double dVCC = dynamic_VCC(depth);

	double x_inches = px2inch(depth, x_pixels);
	double y_inches = px2inch(depth, y_pixels);
	
	return std::make_pair(x_inches, y_inches);
}

double dynamic_VCC(double depth) {
	const double DENOMINATOR_CONSTANT = 504.0;
	const double CONTROL_LENGTH = 2.5;
	
	double dVCC = (CONTROL_LENGTH * depth) / DENOMINATOR_CONSTANT; 
	
	
	//std::cout << "DYNAMIC VCC: " << dVCC << std::endl;
	
	
	return dVCC;
}

double px2inch(double depth, int px_length) {
	double px2in = px_length * dynamic_VCC(depth); 
	
	//std::cout << "px_length: " << px_length << ", depth: " << depth << ", px2in: " << px2in << std::endl;
	
	return px2in;
}



double round_to_nearest_num(double num, double to) {
	return std::round(num / to) * to;
}

double ROLL(const std::vector<std::vector<int>>& lmks) {
	std::vector<int> wrist = lmks[_WRIST];
	std::vector<int> middle = lmks[MIDDLE];
	
	double dx = middle[1] - wrist[1];
	double dy = middle[2] - wrist[2];
	
	double roll = std::atan2(dy, dx);
	double rolldegrees = roll * RAD2DEG;
	rolldegrees += 90;
	rolldegrees = round_to_nearest_num(rolldegrees, 2.5);
	
	return rolldegrees;
}

double YAW(const std::vector<std::vector<int>>& unrolled, double supposed_depth, char hand) {
	std::vector<int> index = unrolled[INDEX];
	std::vector<int> ring = unrolled[RING];
	
	std::vector<int> I2R_vector = {(index[1] - ring[1]), (index[2] - ring[2])};
	double I2R_len = normalize(I2R_vector);
	double full_length = get_I2R_reference_from_depth(supposed_depth);
	double ratio = clamp((I2R_len / full_length), 0, 1);
	
	double yaw = RAD2DEG * std::acos(ratio);
	double multiplier;
	
	//std::cout << "YAW: I2R_len: " << I2R_len << "\nfull_length: " << full_length << "\nratio: " << ratio << std::endl;
	//std::cout << "index[1]: " << index[1] << ", index[2]: " << index[2] <<"\nring[1]: " << ring[1] << ", ring[2]: " << ring[2] << std::endl;
	//print_landmarks(unrolled);
	
	if(hand == 'R') {
		if(index[1] > ring[1]) {
			yaw = (180 - yaw);
		}
		multiplier = -1;
	} else if(hand == 'L') {
		if(index[1] < ring[1]) {
			yaw = (180 - yaw);
		}
		multiplier = 1;
	} else {
		std::cout << "Invalid hand symbol." << std::endl;
		// invalid hand
		return 0;
	}
	
	yaw *= multiplier;
	
	return yaw;
}

double PITCH(const std::vector<std::vector<int>>& lmks, double supposed_depth) {
	std::vector<int> wrist = lmks[_WRIST];
	std::vector<int> middle = lmks[MIDDLE];
	
	std::vector<int> M2W_vector = {(wrist[1] - middle[1]), (wrist[2] - middle[2])};
	double M2W_length = normalize(M2W_vector);
	double full_length = get_M2W_reference_from_depth(supposed_depth);
	double ratio = clamp((M2W_length / full_length), 0, 1);
	
	//std::cout << "PITCH: M2W_len: " << M2W_length << "\nfull_length: " << full_length << "\nratio: " << ratio << std::endl;
	
	double pitch0 = RAD2DEG * std::asin(ratio);
	pitch0 = clamp(pitch0, PITCH0LOWERBOUND, PITCH0UPPERBOUND);
	
	double pitch = interpolate(pitch0, PITCH0LOWERBOUND, PITCH0UPPERBOUND, 90, 0);
	
	return pitch;
}
