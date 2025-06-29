# XR-Starter-Kit
The XR Starter Kit includes a variety of XR-related functionalities for a variety of system types.

-> Camera-Calibration/
	- An instructed guide to camera calibration to create a necessary .yml file for the XR-Starter-Kit.
-> Hand-Tracking/
	- A stereo vision based hand tracking/detection library that determines hands' 3D position and rotation vector, and gestures.
-> Helper-Scripts/
	-> xrimaging.py
		- Python-compatible helper file to handle proper undistortion of images for processing. Includes the following methods:
			- undistort(image, yml_file_path) 		-> NumPy array (OpenCV image, undistorted)
				- Undistorts the image parameter with the given .yml file (first call instance will set permanent the yml data passed for the rest of the program to improve speed).
			- undistort_n(image_arr, yml_file_path) 	-> List of NumPy arrays (List of OpenCV images, undistorted)
				- Undistorts each image in the image array parameter with the given .yml file (same yml data rule applies).
			- undistort_n_opt(image_arr, yml_file_path)	-> List of NumPy arrays (List of OpenCV images, undistorted)
				- Undistorts each image in the image array parameter with the given .yml file with an optimized number of threads (same yml data rule applies).
	-> xrimaging.cpp/h
		- C++-compatible helper file to handle proper undistortion of images for processing. Includes the following methods:
			- cv::Mat undistort(cv::Mat image, std::string yml_file_path)
				- Undistorts the image parameter with the given .yml file (first call instance will set permanent the yml data passed for the rest of the program to improve speed).
			- std::vector<cv::Mat> undistort_n(std::vector<cv::Mat> image_arr, std::string yml_file_path)
				- Undistorts each image in the image array parameter (some container data type containing cv::Mat) with the given .yml file (same yml data rule applies).
			- std::vector<cv::Mat> undistort_n_opt(std::vector<cv::Mat> image_arr, std::string yml_file_path)
				- Undistorts each image in the image array parameter (some container data type containing cv::Mat) with the given .yml file with an optimized number of threads (same yml data rule applies).
