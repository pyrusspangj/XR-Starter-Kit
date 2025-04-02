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

#define ME "/VMIC/VioX Main Camera"
#define VERSION 1.0

namespace fs = boost::filesystem;

const double GRACEPERIOD = 0.500;
const double TOLERANCE = 0.0001;
const double ROOT2 = sqrt(2) + TOLERANCE;
const double THRESHOLD = 0.75;
const double MIDDLE_STABILIZER_THRESHOLD = 0.25;
const double PINCH_THRESHOLD = 32.5;
const double HAND_FILTER_THRESHOLD = 50;
const double DESK_THRESHOLD = 50;
const std::chrono::duration<double> VERIFY = std::chrono::duration<double>(0.675);
const std::chrono::duration<double> CLICKCOOLDOWN = std::chrono::duration<double>(0.125);

string me = ME;
string myname = me.substr(me.find_last_of("/\\") + 1);
string advset = me.substr(0, me.find_last_of("/\\") + 1) + "VioX_AdvancedSettings";
string imghdlr = me.substr(0, me.find_last_of("/\\") + 1) + "VioX_ImageHandler";
string essentials = me.substr(0, me.find_last_of("/\\") + 1) + "essentials.json";
string default_file = me.substr(0, me.find_last_of("/\\") + 1) + "default.json";
string GODOTL = "/VMIC/Godot/VER Data/LData.json";
string GODOTR = "/VMIC/Godot/VER Data/RData.json";

int monitor_width = essentials_get("monitorw", false).get<int>(), monitor_height = essentials_get("monitorh", false).get<int>();
int capture_width = 1, capture_height = 1;

cv::VideoCapture cam1;

Display* display = XOpenDisplay(NULL);

Hand* righthand0 = nullptr;
Hand* lefthand0 = nullptr;

std::pair<std::chrono::time_point<std::chrono::high_resolution_clock>, std::chrono::time_point<std::chrono::high_resolution_clock>> timestamp = std::make_pair(std::chrono::high_resolution_clock::now(), std::chrono::high_resolution_clock::now());
std::chrono::duration<double> dt;

void info() {
	std::string DEP = "DEPRECATED";
	std::string BAD = "BAD";
	std::string OK = "OK";
	std::string GREAT = "GREAT";
	
    cout << "\nVioX Beta " << VERSION << "\n"
         << "me -> " << me << "\n"
         << "myname -> " << myname << "\n"
         << "essentials -> " << essentials << "\n"
         << "hand orient -> " << OK << "\n"
         << "depth percept -> " << OK << "\n"
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

void print_landmarks(const vector<vector<int>>& lmks) {
	for(vector<int> vec : lmks) {
		std::cout << "ID " << vec[0] << ": [" << vec[1] << ", " << vec[2] << "]" << std::endl;
	}
}

double distance(const vector<int>& point1, const vector<int>& point2, bool normalindexing) {
    if (normalindexing) {
        return sqrt(pow(point2[0] - point1[0], 2) + pow(point2[1] - point1[1], 2));
    } else {
        return sqrt(pow(point2[1] - point1[1], 2) + pow(point2[2] - point1[2], 2));
    }
}

double distancef(const float point1[2], const float point2[2]) {
    return sqrt(pow(point2[0] - point1[0], 2) + pow(point2[1] - point1[1], 2));
}

pair<int, int> midpoint(const vector<int>& point1, const vector<int>& point2) {
    int x1 = point1[1], y1 = point1[2];
    int x2 = point2[1], y2 = point2[2];
    int mid_x = (x1 + x2) / 2;
    int mid_y = (y1 + y2) / 2;
    return {mid_x, mid_y};
}

json load_json_file(const string& filepath) {
    try {
        ifstream file(filepath);
        if (!file.is_open()) {
            cerr << "Error opening file: " << filepath << endl;
            return json{};
        }

        json j;
        file >> j;

        // Validate if JSON is parsed correctly
        if (file.fail()) {
            cerr << "Error reading JSON from file: " << filepath << endl;
            return json{};
        }

        return j;
    } catch (const ifstream::failure& e) {
        cerr << "Error loading JSON from " << filepath << ": " << e.what() << endl;
        return json{};
    } catch (const nlohmann::json::parse_error& e) {
        cerr << "JSON parse error in file " << filepath << ": " << e.what() << endl;
        return json{};
    } catch (const std::exception& e) {
        cerr << "Unknown error loading JSON from " << filepath << ": " << e.what() << endl;
        return json{};
    }
}

void write_json_file(const string& filepath, const json& content) {
    try {
        ofstream file(filepath);
        file << setw(2) << content;
    } catch (const ofstream::failure& e) {
        cerr << "Error writing JSON to " << filepath << ": " << e.what() << endl;
    }
}

void write_json_with_lock(const string& filepath, const json& content) {
    int fd = open(filepath.c_str(), O_WRONLY | O_CREAT, 0666);
    if (fd == -1) {
        cerr << "Error opening file for writing: " << filepath << endl;
        return;
    }

    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;

    if (fcntl(fd, F_SETLKW, &lock) == -1) {
        cerr << "Error locking file: " << filepath << endl;
        close(fd);
        return;
    }

    try {
        ofstream file(filepath);
        file << setw(2) << content;
    } catch (const ofstream::failure& e) {
        cerr << "Error writing JSON to " << filepath << ": " << e.what() << endl;
    }

    lock.l_type = F_UNLCK;
    if (fcntl(fd, F_SETLK, &lock) == -1) {
        cerr << "Error unlocking file: " << filepath << endl;
    }

    close(fd);
}

void find_custom_paths(const json& d, const string& current_path, vector<string>& result) {
    for (auto it = d.begin(); it != d.end(); ++it) {
        string path = current_path.empty() ? it.key() : current_path + "," + it.key();
        if (it.value() == "?") {
            result.push_back(path);
        } else if (it->is_object()) {
            find_custom_paths(it.value(), path, result);
        }
    }
}

vector<string> find_custom_paths(const json& d) {
    vector<string> result;
    find_custom_paths(d, "", result);
    return result;
}

json essentials_get(const string& path, bool customs) {
    try {
        if (customs) {
            return find_custom_paths(load_json_file(default_file));
        } else if (path.empty()) {
            return load_json_file(essentials);
        } else {
            json ess = load_json_file(essentials);
            istringstream ss(path);
            string key;
            while (getline(ss, key, ',')) {
                ess = ess[key];
            }
            return ess;
        }
    } catch (json::parse_error& e) {
        cerr << "JSON parse error: " << e.what() << endl;
        return nullptr;
    }
}

void essentials_set(const string& path, const json& value, bool all) {
    if (all) {
        vector<string> userspick = essentials_get("", true).get<vector<string>>();
        vector<json> usersset;
        for (const auto& val : userspick) {
            usersset.push_back(essentials_get(val));
        }
        json defaultvalues = load_json_file(default_file);
        write_json_file(essentials, defaultvalues);
        for (size_t i = 0; i < userspick.size(); ++i) {
            essentials_set(userspick[i], usersset[i]);
        }
    } else {
        json ess = load_json_file(essentials);
        istringstream ss(path);
        string key;
        json* parent = &ess;
        while (getline(ss, key, ',')) {
            parent = &(*parent)[key];
        }
        *parent = value;
        write_json_file(essentials, ess);
    }
}

void power_off() {
    cout << "Thank you for using VioX! Have a good rest of your day <3" << endl;
    essentials_set("", nullptr, true);
}
