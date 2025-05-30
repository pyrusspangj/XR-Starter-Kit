#ifndef GENERICS_H
#define GENERICS_H

#include <utility>
#include <string>
#include <chrono>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>
#include <boost/filesystem.hpp>
#include <opencv2/opencv.hpp>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>

// generic constants
#define VERSION 1.0
#define ME "Hand Tracking - arm64 Linux (u24)"
#define CALIBRATION_INFO "../../Camera Calibration/guided_calibration.yml"
#define CAMERA_INFO "../camera_info.json"
#define HAND_DATA "../hand_data.json"

// hand landmark index constants
#define ID 0
#define X 1
#define Y 2
#define Z 3

// mathematical calculation constants
#define PINCH_THRESHOLD 32.5

// camera interfacing constants
#define MAXCAMS 2
#define CAM1 0
#define CAM2 2

// `setup` constants
#define INCHES 0.0
#define CENTIMETERS 1.0
#define BINOCULAR 1.0
#define STEREO 0.0


using json = nlohmann::json;
namespace fs = boost::filesystem;

extern cv::Mat camera_matrix, dist_coeffs;
extern double cx, cy;
extern double fx, fy;

extern std::map<std::string, double> setup;
  

enum FingerParts {
    WRIST = 0,
    THUMB_CMC,
    THUMB_MCP,
    THUMB_IP,
    THUMB_TIP,
    INDEX_FINGER_MCP,
    INDEX_FINGER_PIP,
    INDEX_FINGER_DIP,
    INDEX_FINGER_TIP,
    MIDDLE_FINGER_MCP,
    MIDDLE_FINGER_PIP,
    MIDDLE_FINGER_DIP,
    MIDDLE_FINGER_TIP,
    RING_FINGER_MCP,
    RING_FINGER_PIP,
    RING_FINGER_DIP,
    RING_FINGER_TIP,
    PINKY_MCP,
    PINKY_PIP,
    PINKY_DIP,
    PINKY_TIP
};


extern std::string me;
extern std::string my_path;

extern int capture_width;
extern int capture_height;

extern std::pair<std::chrono::time_point<std::chrono::high_resolution_clock>, std::chrono::time_point<std::chrono::high_resolution_clock>> timestamp;
extern std::chrono::duration<double> dt;

// methods
void info();

void filter_timestamp();
double get_delta_time();

double distance(const std::vector<int>& point1, const std::vector<int>& point2, bool normalindexing = false);
double distancef(const float point1[2], const float point2[2]);

std::pair<int, int> midpoint(const std::vector<int>& point1, const std::vector<int>& point2);

nlohmann::json load_json_file(const std::string& filepath);
void write_json_file(const std::string& filepath, const nlohmann::json& content);
void write_json_with_lock(const std::string& filepath, const nlohmann::json& content);

void power_off();

#endif // GENERICS_H
