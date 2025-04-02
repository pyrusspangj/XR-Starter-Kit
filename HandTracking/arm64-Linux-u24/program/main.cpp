#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

#include <opencv2/opencv.hpp>
#include <Python.h>
#include <numpy/arrayobject.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <unistd.h>
#include <chrono>
#include <ctime>
#include <cstdlib>

#include "Generics.h"
#include "Desktop/Desktop.h"
#include "HandProc.h"
#include "InterfaceController.h"

#define ME "Hand Tracking - arm64 Linux (u24)"

#define MAXCAMS 6
#define CAM1 0
#define CAM2 2
#define CAM3 4
#define CAM4 6
#define CAM5 8
#define CAM6 10

// infosheet
unsigned short cams;
unsigned short FOV;
unsigned short cams_distance;
char unit[2];
float detectconf;
float trackconf;


cv::VideoCapture cams[MAXCAMS];
unsigned short cams_system_index[MAXCAMS] = {0, 2, 4, 6, 8, 10};
double** cam_matrix;


double** estimate_camera_matrix(const cv::Mat &image, int FOVx_deg) {
	int width = image.cols;
	int height = image.rows;
	float ratio = height/width;
	
	double FOVx_rad = FOV_degx * (CV_PI / 180.0);
	double FOVy_rad = (FOV_degx * ratio) * (CV_PI / 180.0);
	
	double focal_length_x = width / (2.0 * tan(FOVx_rad / 2.0));
	double focal_length_y = height / (2.0 * tan(FOVy_rad / 2.0));
	
	double cx = width / 2.0;
	double cy = height / 2.0;
	
	double** K = new int*[3];
	for(int i=0; i<3; i++) {
		K[i] = new double[3];
	}
	
	K[0][0] = focal_length_x;
	K[0][1] = 0;
	K[0][2] = cx;
	
	K[1][0] = 0;
	K[1][1] = focal_length_y;
	K[1][2] = cy;
	
	K[2][0] = 0;
	K[2][1] = 0;
	K[2][2] = 1;
	
	return K;
}

cv::Mat undistorted_mat(const cv::Mat &distorted, double** matrix) {
	cv::Mat K(3, 3, CV_64F);
	for(int i=0; i<3; i++)
		for(int j=0; j<3; j++)
			K.at<double>(i, j) = matrix[i][j];
			
}


bool ready() {
	
	for(cv::VideoCapture cam : cams)
		if(!cam.isOpened())
			return false;
	
	return true;
	
}

void init_initz() {
    // General.cpp method
    //info();
	
	for(int i=0; i<cams; i++) {
		cams[i].open(cams_system_index[i] + cv::CAP_V4L2);
	}
	
	cv::Mat frame;
	cams[0] >> frame;
	cam_matrix = estimate_camera_matrix(frame, FOV);
	
	/*		test statements. do what you will further on.
    cap1.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
    cap1.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);
    */
      
    // General.cpp method  
    //timestamp.second = std::chrono::high_resolution_clock::now();
}

void stop() {
	
	for(cv::VideoCapture cam : cams)
		cam.release();
	
    cv::destroyAllWindows();
}

void scan() {
    PythonManager pymgr("mediaproc");
    
    //unsure if we should continue using this
    //HandsMaker eye1(CAM1);
    
    while (ready()) {
        // General.cpp usage
        //std::cout << "---------- BEGIN ----------" << std::endl;
        //filter_timestamp();

        cv::Mat frames[cams];
        for(int i=0; i<cams; i++) {
			cams[i] >> frames[i];
			
			
			// later you need to create a run.sh parameter that accounts for the camera rotations (should all be equal)
			//cv::rotate(frame1, frame1, cv::ROTATE_90_CLOCKWISE);
			//cv::rotate(frame1, frame1, cv::ROTATE_180);
			
			// General.cpp information
			capture_height = frames[0].rows;
			capture_width = frames[0].cols;
			
			std::cout << capture_width << std::endl;
			
			if(frames[i].empty()) {
				std::cout << "Frame from Camera #" << (i+1) << " is empty. This iteration will break." << std::endl;
				break;
			}
					
			struct Hands hands[cams];
			
			if(send_frames) {
				
				for(int i=0; i<cams; i++) {
					hands[i] = pymgr.sendf(frame1);
				}
				
			}
		}
		
		
        
		cv::imshow("CAM1", frame1);

        if (cv::waitKey(1) == 27) { // Exit on ESC
            break;
        } 

		std::cout << "---------- END ----------" << std::endl;
		//std::cout << "RIGHTHAND0 AT END OF MAIN LOOP: " << righthand0 << std::endl;
        //auto end = std::chrono::high_resolution_clock::now(); // End time
        //std::chrono::duration<double> elapsed = end - start; // Elapsed time
        //std::cout << "Time between iterations: " << elapsed.count() << " seconds" << std::endl; // Print elapsed time
    }
    
}

void start() {
    init_initz();
    scan();
}

int main(int argc, char* argv[]) {
	
	for(int i=1; i<argc; i++) {
		switch(i) {
			
			case 1:
				cams = std::atoi(argv[i]);
				break;
			case 2:
				FOV = std::atoi(argv[i]);
				break;
			case 3:
				cams_distance = std::atoi(argv[i]);
				break;
			case 4:
				unit[0] = argv[i][0];
				unit[1] = argv[i][1];
				break;
			case 5:
				detectconf = std:atof(argv[i]);
				break;
			case 6:
				trackconf = std::atof(argv[i]);
				break;
			default:
				std::cout << "Incompatible `argv` index `" << i << "` for " << ME << std::endl;
			
		}
	}
	
    start();

    return 0;
}
