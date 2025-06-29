#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

#include <iostream>

#include "Generics.h"
#include "handtracking.h"

#define CAMERA1 0
#define CAMERA2 1

cv::VideoCapture cam1(CAM1);
cv::VideoCapture cam2(CAM2);

void start() {
	
	while(true) {
		cv::Mat frame1, frame2;
		cam1 >> frame1;
		cam2 >> frame2;

		if(frame1.empty() || frame2.empty()) continue;

		struct Hands hands = get_hands(frame1, frame2, "~/Downloads/guided_calibration.yml");

		if(cv::waitKey(1) == 27) break;	
	}
	
}


void stop() {

	cam1.release();
	cam2.release();

	cv::destroyAllWindows();

}


int main() {
	
	start();
	stop();

}
