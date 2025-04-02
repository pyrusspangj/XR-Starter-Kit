#ifndef HAND_H
#define HAND_H

#include "Fingers.h"
#include "../VXMath.h"
#include <string>
#include <vector>
#include <array>
#include <memory>
#include <cmath> 
#include <map>

class Hand {
public:

    enum TYPE {
        LEFT,
        RIGHT
    };

    static const std::array<std::string, 6> gests;
    static const float M2Wk;  // conversion constants for pixel length to depth in inches
    static const float I2Rk;  // conversion constants for pixel length to depth in inches
    static const float VCC;   // VioX conversion constant
        
    double pitch, yaw, roll;

	double x_inches, y_inches;
    double depth, dz;
    double dx, dy;
    double last_depth;
    std::string current_gest, last_gest;
    std::vector<Finger*> current_prom, last_prom;
    std::vector<std::vector<float>> raw_points;
    std::vector<std::vector<int>> points;
    TYPE hand;
    TYPE true_hand;
    bool showing_palm;
    std::unique_ptr<Thumb> thumb;
    std::unique_ptr<Index> index;
    std::unique_ptr<Middle> middle;
    std::unique_ptr<Ring> ring;
    std::unique_ptr<Pinky> pinky;
    std::vector<std::vector<int>> thesquare;
    std::vector<std::string> verification;
    std::string new_gest;
    int prediction_no;

    Hand(std::vector<std::vector<int>> points, TYPE hand, const std::string& truetype);
    void update(std::vector<std::vector<int>> points, const std::string& truetype);
    std::vector<std::vector<float>> make_raw_points();
    std::vector<std::vector<int>> build_square();
    void initialize_fingers();
    std::pair<std::string, std::vector<Finger*>> determine_gesture();
    bool resembles_pinch(std::vector<Finger*> proms);
    std::string verify();
    TYPE identify_true_handtype(const std::string& type);
    bool shows_palm();
    void stabililze_middle_knuckle();
    
    bool predict(float comrP[2], int hpts[21][3]);
    
    TYPE true_handtype();
    int num_prominent_fingers();
    bool is_making_gesture();
    std::string get_current_gesture();
    std::string get_last_gesture();
    std::vector<Finger*> get_prominent_fingers();
    Thumb* get_thumb();
    Index* get_index();
    Middle* get_middle();
    Ring* get_ring();
    Pinky* get_pinky();
    double get_x_inches();
    double get_y_inches();
    double get_depth();
    double get_dz();
    double get_dy();
    double get_dx();
    double get_pitch();
    double get_yaw();
    double get_roll();
    
    static double distanceh(Finger* thumb, Finger* finger2);
};

#endif // HAND_H
