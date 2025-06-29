#include <iostream>
#include <thread>
#include <pthread.h>
#include <mutex>
#include <vector>
#include <opencv2/opencv.hpp>


const std::string K_KEY = "K";
const std::string DIST_COEFFS_KEY = "dist";

const unsigned int cores = std::thread::hardware_concurrency();


extern cv::FileStorage XRI_fs;
extern cv::Mat XRI_K, XRI_dist_coeffs, XRI_new_K;


extern pthread_mutex_t undistort_mutex;
extern std::vector<cv::Mat> undistorted_optimized_images;


struct WorkerInfo {
	std::vector<cv::Mat> my_images;
	std::string yml_file_path;
	int start;
	int end;
};


void read_yml(std::string yml_file_path);
void scan_for_file(std::string yml_file_path);


cv::Mat undistort(cv::Mat image, std::string yml_file_path);
std::vector<cv::Mat> undistort_n(const std::vector<cv::Mat>& image_arr, std::string yml_file_path);

void* undistort_n_worker(void* args);
std::vector<cv::Mat> undistort_n_opt(const std::vector<cv::Mat>& image_arr, std::string yml_file_path);

