#ifndef GENERICS_H
#define GENERICS_H

#include <utility>
#include <string>
#include <chrono>
#include <vector>
#include <nlohmann/json.hpp>
#include <boost/filesystem.hpp>
#include <opencv2/opencv.hpp>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>

class Hand;

using json = nlohmann::json;
using namespace std;
namespace fs = boost::filesystem;

// Constants
extern const double GRACEPERIOD;
extern const double TOLERANCE;
extern const double ROOT2;
extern const double THRESHOLD;
extern const double MIDDLE_STABILIZER_THRESHOLD;
extern const double PINCH_THRESHOLD;
extern const double HAND_FILTER_THRESHOLD;
extern const double DESK_THRESHOLD;
extern const std::chrono::duration<double> VERIFY;
extern const std::chrono::duration<double> CLICKCOOLDOWN;

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

extern string me;
extern string myname;
extern string advset;
extern string imghdlr;
extern string essentials;
extern string default_file;
extern string GODOTL;
extern string GODOTR;

extern int monitor_width;
extern int monitor_height;
extern int capture_width;
extern int capture_height;

extern cv::VideoCapture cam1;

extern Display* display;

extern Hand* righthand0;
extern Hand* lefthand0;

extern std::pair<std::chrono::time_point<std::chrono::high_resolution_clock>, std::chrono::time_point<std::chrono::high_resolution_clock>> timestamp;
extern std::chrono::duration<double> dt;

void info();
void filter_timestamp();
double get_delta_time();
void print_landmarks(const vector<vector<int>>& lmks);
double distance(const vector<int>& point1, const vector<int>& point2, bool normalindexing = false);
double distancef(const float point1[2], const float point2[2]);
pair<int, int> midpoint(const vector<int>& point1, const vector<int>& point2);
json load_json_file(const string& filepath);
void write_json_file(const string& filepath, const json& content);
void write_json_with_lock(const string& filepath, const json& content);
vector<string> find_custom_paths(const json& d);
json essentials_get(const string& path = "", bool customs = false);
void essentials_set(const string& path = "", const json& value = nullptr, bool all = false);
void power_off();

#endif // GENERICS_H
