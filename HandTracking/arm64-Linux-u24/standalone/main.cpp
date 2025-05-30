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
#include "HandProc.h"




cv::VideoCapture cams[MAXCAMS];
unsigned short cams_system_index[MAXCAMS] = {0, 2};



void init_camera_info() {
	cv::FileStorage fs(CALIBRATION_INFO, cv::FileStorage::READ);
	fs["K"] >> camera_matrix;
	fs["dist"] >> dist_coeffs;
	fs.release();
	
	// set the cx, cy in Generics.cpp
	std::cout << "WARNING: If you receive a segmentation fault here, it is likely because you never calibrated your camera, and no calibration information can be found!" << std::endl;
	cx = camera_matrix.at<double>(0, 2);
	cy = camera_matrix.at<double>(1, 2);
	
	// set the fx, fy in Generics.cpp
	fx = camera_matrix.at<double>(0, 0);
	fy = camera_matrix.at<double>(1, 1);
	
	// --------------------------------------------------------
	// read the camera_info.json
	nlohmann::json j = load_json_file(CAMERA_INFO);
	
	try {
		setup["root"] = j["setup"]["root"].get<double>();
		setup["images"] = j["setup"]["images"].get<double>();
		setup["binocular"] = (double)((int)(j["setup"]["binocular"].get<bool>()));
		setup["fov"] = j["setup"]["fov"].get<double>();
		setup["distance"] = j["setup"]["distance"].get<double>();
		
		switch(j["setup"]["rotation"].get<int>()) {
			case 90: setup["rotation"] = (double)cv::ROTATE_90_COUNTERCLOCKWISE; break;
			case 180: setup["rotation"] = (double)cv::ROTATE_180; break;
			case 270: setup["rotation"] = (double)cv::ROTATE_90_CLOCKWISE; break;
			default: setup["rotation"] = -1.0;
		}
	} catch(...) {
		std::cerr << "ERROR in `init_camera_info()`, did you forget to fill out 'camera_info.json'?" << std::endl;
	}
	
}


cv::Mat undistorted_image(const cv::Mat &distorted) {
	cv::Mat new_matrix = cv::getOptimalNewCameraMatrix(camera_matrix, dist_coeffs, distorted.size(), 0.0);
	cv::Mat undistorted;
	cv::undistort(distorted, undistorted, camera_matrix, dist_coeffs, new_matrix);
	
	return undistorted;
}


void init_initz() {
    // General.cpp method
    //info();
	
	cams[0].open(cams_system_index[0] + cv::CAP_V4L2);
	
	if(setup["binocular"] != BINOCULAR) {
		cams[1].open(cams_system_index[1] + cv::CAP_V4L2);
	}
	
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
    
    while(true) {
        // General.cpp usage
        //std::cout << "---------- BEGIN ----------" << std::endl;
        //filter_timestamp();

        cv::Mat frames[MAXCAMS];
        cv::Mat frame;
        
        struct TemplateHands template_hands[MAXCAMS];
        
        // image processing dependent on camera setup
		if(setup["binocular"] == BINOCULAR) {
			cams[0] >> frame;
			
			if(frame.empty()) {
				std::cerr << "Frame from BINOCULAR camera is empty!" << std::endl;
				continue;
			}
			
			capture_height = frame.rows;
			capture_width = frame.cols / 2;
			
			// left image
			cv::Rect_ roiL(0, 0, capture_width, capture_height);
			if(setup["rotation"] != -1.0) {
				cv::Mat rotated;
				cv::rotate(frame(roiL), rotated, (int)setup["rotation"]);
				frames[0] = undistorted_image(rotated);
			} else 							frames[0] = undistorted_image(frame(roiL));
			
			// right image
			cv::Rect_ roiR(capture_width, 0, frame.cols, frame.rows);
			if(setup["rotation"] != -1.0) {
				cv::Mat rotated;
				cv::rotate(frame(roiR), rotated, (int)setup["rotation"]);
				frames[1] = undistorted_image(rotated);
			} else 							frames[1] = undistorted_image(frame(roiR));
			
			TemplateHands th1, th2;
			
			th1 = pymgr.sendf(frames[0]);
			th2 = pymgr.sendf(frames[1]);
			
			if(th1.init_status == PROPERLY_INITIALIZED &&
				th2.init_status == PROPERLY_INITIALIZED) {
				template_hands[0] = th1;
				template_hands[1] = th2;
			} else continue;
		} else {
			for(int i=0; i<MAXCAMS; i++) {
				if(!cams[i].isOpened()) break;
				
				cams[i] >> frame;
				
				if(frame.empty()) {
					std::cout << "Frame from Camera #" << (i+1) << " is empty. This iteration will break." << std::endl;
					break;
				}
				
				capture_height = frame.rows;
				capture_width = frame.cols;
				
				if(setup["rotation"] != -1.0) {
					cv::Mat rotated;
					cv::rotate(frame, rotated, (int)setup["rotation"]);
					frames[i] = undistorted_image(rotated);
				} else 							frames[i] = undistorted_image(frame);
						
				TemplateHands th = pymgr.sendf(frames[i]);
			
				if(th.init_status == PROPERLY_INITIALIZED) {
					template_hands[i] = th;
				}
			}
		}
		
		process_all_hands(template_hands);
        
        if (cv::waitKey(1) == 27) { // Exit on ESC
            break;
        } 
        
        // write hand data to hand_data.json
        write_hands();

		std::cout << "---------- END ----------" << std::endl;
		//std::cout << "RIGHTHAND0 AT END OF MAIN LOOP: " << righthand0 << std::endl;
        //auto end = std::chrono::high_resolution_clock::now(); // End time
        //std::chrono::duration<double> elapsed = end - start; // Elapsed time
        //std::cout << "Time between iterations: " << elapsed.count() << " seconds" << std::endl; // Print elapsed time
    }
    
}

void start() {
    init_initz();
    init_camera_info();
    scan();
}

int main(int argc, char* argv[]) {
	
    start();

    return 0;
}
