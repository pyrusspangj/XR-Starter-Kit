#include "xrimaging.h"

// compilation: g++ -std=c++17 -O2 -c xrimaging.cpp `pkg-config --cflags opencv4` -o xrimaging.o -pthread


cv::FileStorage XRI_fs;


cv::Mat XRI_K;
cv::Mat XRI_dist_coeffs;
cv::Mat XRI_new_K;


void read_yml(std::string yml_file_path) {
	try {
		XRI_fs.open(yml_file_path, cv::FileStorage::READ);
		XRI_fs[K_KEY] >> XRI_K;
		XRI_fs[DIST_COEFFS_KEY] >> XRI_dist_coeffs;
	} catch(...) {
		std::cout << "ERROR, cannot find file '" << yml_file_path << "'." << std::endl;
	}
}


void scan_for_file(std::string yml_file_path) {
	if(!XRI_fs.isOpened()) read_yml(yml_file_path);
}


// ---


cv::Mat undistort(cv::Mat image, std::string yml_file_path) {
	if(XRI_new_K.empty())
		XRI_new_K = cv::getOptimalNewCameraMatrix(XRI_K, XRI_dist_coeffs, image.size(), 0.0);
	cv::Mat undistorted;
	cv::undistort(image, undistorted, XRI_K, XRI_dist_coeffs, XRI_new_K);
	return undistorted;
}


std::vector<cv::Mat> undistort_n(const std::vector<cv::Mat>& image_arr, std::string yml_file_path) {
	std::vector<cv::Mat> images;
	for(const cv::Mat& image : image_arr) {
		images.push_back(undistort(image, yml_file_path));
	}
	return images;
}


// ---


pthread_mutex_t undistort_mutex = PTHREAD_MUTEX_INITIALIZER;
std::vector<cv::Mat> undistorted_optimized_images;


void* undistort_n_worker(void* args) {
	WorkerInfo info = *(WorkerInfo*)args;

	std::vector<cv::Mat> result = undistort_n(info.my_images, info.yml_file_path);

	pthread_mutex_lock(&undistort_mutex);
	for(int i=0; i<result.size(); i++) {
		undistorted_optimized_images[info.start + i] = result[i];
	}
	pthread_mutex_unlock(&undistort_mutex);

	return nullptr;
}


std::vector<cv::Mat> undistort_n_opt(const std::vector<cv::Mat>& image_arr, std::string yml_file_path) {
	int total_images = image_arr.size();
	int num_threads = std::min(total_images, (int)cores);

	undistorted_optimized_images.resize(total_images);

	std::vector<pthread_t> imaging_threads(num_threads);
	std::vector<WorkerInfo> thread_data(num_threads);

	int chunk_size = total_images / num_threads;

	for(int i=0; i<num_threads; i++) {
		int start = i * chunk_size;
		int end = (i == num_threads-1) ? total_images : start + chunk_size;

		thread_data[i].my_images = std::vector<cv::Mat>(image_arr.begin() + start, image_arr.begin() + end);
		thread_data[i].yml_file_path = yml_file_path;
		thread_data[i].start = start;
		thread_data[i].end = end;

		pthread_create(&imaging_threads[i], nullptr, undistort_n_worker, &thread_data[i]);
	}

	for(int i=0; i<num_threads; i++) {
		pthread_join(imaging_threads[i], nullptr);
	}

	return undistorted_optimized_images;
}
