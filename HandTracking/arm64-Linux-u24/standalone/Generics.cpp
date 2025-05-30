#include "Generics.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <ctime>
#include <chrono>
#include <boost/filesystem.hpp>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

namespace fs = boost::filesystem;

cv::Mat camera_matrix, dist_coeffs;
double cx = 0, cy = 0;
double fx = 0, fy = 0;

std::map<std::string, double> setup{};



std::string me = ME;
std::string my_path = fs::current_path().string();

int capture_width = 1, capture_height = 1;

std::pair<std::chrono::time_point<std::chrono::high_resolution_clock>, std::chrono::time_point<std::chrono::high_resolution_clock>> timestamp = std::make_pair(std::chrono::high_resolution_clock::now(), std::chrono::high_resolution_clock::now());
std::chrono::duration<double> dt;

void info() {
	std::string DEP = "DEPRECATED";
	std::string BAD = "BAD";
	std::string OK = "OK";
	std::string GREAT = "GREAT";
	
    std::cout << "me -> " << me << "\n"
         << "may it be by the grace of God\n\n";
}

void filter_timestamp() {
    timestamp.first = timestamp.second;
    timestamp.second = std::chrono::high_resolution_clock::now();
    dt = timestamp.second - timestamp.first;
    std::cout << "Time difference: " << dt.count() << " seconds\n";
}

double get_delta_time() {
	return dt.count();
}

double distance(const std::vector<int>& point1, const std::vector<int>& point2, bool normalindexing) {
    if (normalindexing) {
        return std::sqrt(std::pow(point2[0] - point1[0], 2) + std::pow(point2[1] - point1[1], 2));
    } else {
        return std::sqrt(std::pow(point2[1] - point1[1], 2) + std::pow(point2[2] - point1[2], 2));
    }
}

double distancef(const float point1[2], const float point2[2]) {
    return std::sqrt(std::pow(point2[0] - point1[0], 2) + std::pow(point2[1] - point1[1], 2));
}

std::pair<int, int> midpoint(const std::vector<int>& point1, const std::vector<int>& point2) {
    int x1 = point1[1], y1 = point1[2];
    int x2 = point2[1], y2 = point2[2];
    int mid_x = (x1 + x2) / 2;
    int mid_y = (y1 + y2) / 2;
    return {mid_x, mid_y};
}

json load_json_file(const std::string& filepath) {
    try {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Error opening file: " << filepath << std::endl;
            return json{};
        }

        json j;
        file >> j;

        if (file.fail()) {
            std::cerr << "Error reading JSON from file: " << filepath << std::endl;
            return json{};
        }

        return j;
    } catch (const std::ifstream::failure& e) {
        std::cerr << "Error loading JSON from " << filepath << ": " << e.what() << std::endl;
        return json{};
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "JSON parse error in file " << filepath << ": " << e.what() << std::endl;
        return json{};
    } catch (const std::exception& e) {
        std::cerr << "Unknown error loading JSON from " << filepath << ": " << e.what() << std::endl;
        return json{};
    }
}

void write_json_file(const std::string& filepath, const json& content) {
    try {
        std::ofstream file(filepath);
        file << std::setw(2) << content;
    } catch (const std::ofstream::failure& e) {
        std::cerr << "Error writing JSON to " << filepath << ": " << e.what() << std::endl;
    }
}

void write_json_with_lock(const std::string& filepath, const json& content) {
    int fd = open(filepath.c_str(), O_WRONLY | O_CREAT, 0666);
    if (fd == -1) {
        std::cerr << "Error opening file for writing: " << filepath << std::endl;
        return;
    }

    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;

    if (fcntl(fd, F_SETLKW, &lock) == -1) {
        std::cerr << "Error locking file: " << filepath << std::endl;
        close(fd);
        return;
    }

    try {
        std::ofstream file(filepath);
        file << std::setw(2) << content;
    } catch (const std::ofstream::failure& e) {
        std::cerr << "Error writing JSON to " << filepath << ": " << e.what() << std::endl;
    }

    lock.l_type = F_UNLCK;
    if (fcntl(fd, F_SETLK, &lock) == -1) {
        std::cerr << "Error unlocking file: " << filepath << std::endl;
    }

    close(fd);
}


void power_off() {
    std::cout << "Thank you for using VioX! Have a good rest of your day <3" << std::endl;
}
