#include "Hand.h"
#include "../Generics.h"
#include <cmath>
#include <algorithm>
#include <iterator>
#include <iostream>  // Add for debug output
#include <stdexcept>

#define PREDICTION_LIMIT 10
#define PREDICTION_RATE 100/PREDICTION_LIMIT


const std::array<std::string, 6> Hand::gests = {"FIST", "POINTERUP", "TWOFINGERUP", "THREEFINGERUP", "THREEFINGERUP", "HANDUP"};

const float Hand::M2Wk = 884;        // conversion constants for pixel length to depth in inches
const float Hand::I2Rk = 372;        // conversion constants for pixel length to depth in inches
//const float Hand::VCC = 0.01875;		// VIOX conversion constant
const float Hand::VCC = 0.06171244;

// BETTER ESTIMATE FOR PIXELS PER INCH IS 16.204188 EXACTLY ~= 16.2 PIXELS PER INCH
// THIS MEANS THAT EVERY PIXEL IS 0.06171244 OF AN INCH ~= 0.062 INCHES PER PIXEL

// THIS MEANS THAT 2 FEET (24 INCHES) IN FRONT IS ~388.8 PIXELS
// THIS MEANS THAT 2 FEET AND 2 INCHES (26 INCHES) IN FRONT IS ~421.2 PIXELS
//
// 1m (GODOT ENGINE UNIT OF MEASUREMENT) == 13 INCHES
//
// var capture_width = 640
// var half_width = capture_width / 2
// GODOT HAND PLACEMENT == ( (X_PX_POS - half_width) * 0.06171244 ) / 13

Hand::Hand(std::vector<std::vector<int>> points, TYPE hand, const std::string& truetype) 
    : points(points), hand(hand), thumb(nullptr), index(nullptr), middle(nullptr), ring(nullptr), pinky(nullptr) {
    update(points, truetype);
    this->prediction_no = 0;
}

void Hand::update(std::vector<std::vector<int>> points, const std::string& truetype) {
    //std::cout << "IN: void Hand::update()" << std::endl;
    this->last_gest = get_current_gesture();
    this->last_prom = get_prominent_fingers();
    this->points = points;
    
    char trueletter = truetype[0];
    std::array<double, 6> orientation_information = rotation_get(points, trueletter);
    this->roll = orientation_information[0];
    this->yaw = orientation_information[1];
    this->pitch = orientation_information[2];
    this->depth = orientation_information[3];
    this->x_inches = orientation_information[4];
    this->y_inches = orientation_information[5];
    
    this->dx = 0;
    this->dy = 0;
    initialize_fingers();
    
    this->thesquare = build_square();
    this->current_gest = verify();
    
    //stabililze_middle_knuckle();
    
    float mid_hand_x = points[FingerParts::WRIST][1];
    float mid_hand_y = (points[FingerParts::WRIST][2] + points[FingerParts::MIDDLE_FINGER_MCP][2]) / 2;

	std::cout << "Hand Gesture: " << this->get_current_gesture() << std::endl;
    std::cout << "Hand Depth: " << this->depth << std::endl;
    std::cout << "Pitch, Yaw, Roll: [" << this->pitch << ", " << this->yaw << ", " << this->roll << "]" << std::endl;
	std::cout << trueletter << " Hand (x,y): (" << mid_hand_x << ", " << mid_hand_y << ")" << std::endl;
	
	
	// extra paper to work with goes below this line
	//double distance_px_imcp2pmcp = distance(points[FingerParts::INDEX_FINGER_MCP], points[FingerParts::PINKY_MCP]);
	//const double static_inches_length = 2.5;
	
	//std::cout << "hand knuckle distance at " << this->depth << " inches depth is " << distance_px_imcp2pmcp << " pixels." << std::endl;
	
	std::cout << "X Inches: " << this->x_inches << "	Y Inches: " << this->y_inches << std::endl;
}

std::vector<std::vector<int>> Hand::build_square() {
	float BASE_OFFSET = 25;
	int OFFSET = BASE_OFFSET / (this->get_depth() / 5.5);
	// Check if we have enough points to build the palm shape
    if (points.size() < 21) {
        return {{0, 0}, {0, 0}};
    }

    // Collect the palm points based on Mediapipe's hand structure
    std::vector<std::vector<int>> palm_points = {
        points[FingerParts::WRIST],
        points[FingerParts::THUMB_CMC],
        points[FingerParts::THUMB_MCP],
        points[FingerParts::INDEX_FINGER_MCP],
        points[FingerParts::MIDDLE_FINGER_MCP],
        points[FingerParts::RING_FINGER_MCP],
        points[FingerParts::PINKY_MCP]
    };

    // Find the minimum and maximum x and y values to create the bounding shape
    int x_min = std::numeric_limits<int>::max();
    int y_min = std::numeric_limits<int>::max();
    int x_max = std::numeric_limits<int>::min();
    int y_max = std::numeric_limits<int>::min();

    for (const auto& point : palm_points) {
        x_min = std::min(x_min, point[1]);
        y_min = std::min(y_min, point[2]);
        x_max = std::max(x_max, point[1]);
        y_max = std::max(y_max, point[2]);
    }

    // Create a convex hull or a similar bounding shape around the palm points
    // For simplicity, we'll return a bounding box here. You can replace this with a convex hull algorithm if needed.
    std::vector<std::vector<int>> bounding_shape = {
        {std::max(x_min - OFFSET, 0), std::max(y_min - OFFSET, 0)},  // Top-left corner
        {std::min(x_max + OFFSET, capture_width), std::max(y_min - OFFSET, 0)},  // Top-right corner
        {std::min(x_max + OFFSET, capture_width), std::min(y_max + OFFSET, capture_height)},  // Bottom-right corner
        {std::max(x_min - OFFSET, 0), std::min(y_max + OFFSET, capture_height)}   // Bottom-left corner
    };

    // Optional: Uncomment this section if you want to use a convex hull for better fitting around the palm
    /*
    std::vector<cv::Point> cv_points;
    for (const auto& point : palm_points) {
        cv_points.emplace_back(cv::Point(point[1], point[2]));
    }

    std::vector<cv::Point> hull;
    cv::convexHull(cv_points, hull);

    std::vector<std::vector<int>> bounding_shape;
    for (const auto& pt : hull) {
        bounding_shape.push_back({0, pt.x, pt.y});  // The first element is placeholder for compatibility
    }
    */

    return bounding_shape;
}

void Hand::initialize_fingers() {
    //std::cout << "INSIDE INITIALIZE_FINGERS" << std::endl;

    if (thumb == nullptr) {
        thumb = std::make_unique<Thumb>(points[FingerParts::THUMB_CMC], points[FingerParts::THUMB_MCP], points[FingerParts::THUMB_IP], points[FingerParts::THUMB_TIP], this);
        //std::cout << "Initialized thumb" << std::endl;
    } else {
        thumb->fupdate(points[FingerParts::THUMB_CMC], points[FingerParts::THUMB_MCP], points[FingerParts::THUMB_IP], points[FingerParts::THUMB_TIP]);
        //std::cout << "Updated thumb" << std::endl;
    }

    if (index == nullptr) {
        index = std::make_unique<Index>(points[FingerParts::INDEX_FINGER_MCP], points[FingerParts::INDEX_FINGER_PIP], points[FingerParts::INDEX_FINGER_DIP], points[FingerParts::INDEX_FINGER_TIP], this);
        //std::cout << "Initialized index" << std::endl;
    } else {
        index->fupdate(points[FingerParts::INDEX_FINGER_MCP], points[FingerParts::INDEX_FINGER_PIP], points[FingerParts::INDEX_FINGER_DIP], points[FingerParts::INDEX_FINGER_TIP]);
        //std::cout << "Updated index" << std::endl;
    }

    if (middle == nullptr) {
        middle = std::make_unique<Middle>(points[FingerParts::MIDDLE_FINGER_MCP], points[FingerParts::MIDDLE_FINGER_PIP], points[FingerParts::MIDDLE_FINGER_DIP], points[FingerParts::MIDDLE_FINGER_TIP], this);
        //std::cout << "Initialized middle" << std::endl;
    } else {
        middle->fupdate(points[FingerParts::MIDDLE_FINGER_MCP], points[FingerParts::MIDDLE_FINGER_PIP], points[FingerParts::MIDDLE_FINGER_DIP], points[FingerParts::MIDDLE_FINGER_TIP]);
        //std::cout << "Updated middle" << std::endl;
    }

    if (ring == nullptr) {
        ring = std::make_unique<Ring>(points[FingerParts::RING_FINGER_MCP], points[FingerParts::RING_FINGER_PIP], points[FingerParts::RING_FINGER_DIP], points[FingerParts::RING_FINGER_TIP], this);
        //std::cout << "Initialized ring" << std::endl;
    } else {
        ring->fupdate(points[FingerParts::RING_FINGER_MCP], points[FingerParts::RING_FINGER_PIP], points[FingerParts::RING_FINGER_DIP], points[FingerParts::RING_FINGER_TIP]);
        //std::cout << "Updated ring" << std::endl;
    }

    if (pinky == nullptr) {
        pinky = std::make_unique<Pinky>(points[FingerParts::PINKY_MCP], points[FingerParts::PINKY_PIP], points[FingerParts::PINKY_DIP], points[FingerParts::PINKY_TIP], this);
        //std::cout << "Initialized pinky" << std::endl;
    } else {
        pinky->fupdate(points[FingerParts::PINKY_MCP], points[FingerParts::PINKY_PIP], points[FingerParts::PINKY_DIP], points[FingerParts::PINKY_TIP]);
        //std::cout << "Updated pinky" << std::endl;
    }

    //std::cout << "EXITING INITIALIZE_FINGERS" << std::endl;
    this->dx /= 5;
    this->dy /= 5;		// the finger's add up their dx and dy's to the hand's dx and dy, so here we take the average.
}


std::pair<std::string, std::vector<Finger*>> Hand::determine_gesture() {
    std::vector<Finger*> proms;
    if (thumb->is_prominent()) proms.push_back(thumb.get());
    if (index->is_prominent()) proms.push_back(index.get());
    if (middle->is_prominent()) proms.push_back(middle.get());
    if (ring->is_prominent()) proms.push_back(ring.get());
    if (pinky->is_prominent()) proms.push_back(pinky.get());

    int num_prominent = proms.size();
    if (resembles_pinch(proms)) {
        for (int i = 1; i < num_prominent; ++i) {
            if (Hand::distanceh(thumb.get(), proms[i]) <= PINCH_THRESHOLD) {
                return {"PINCH", proms};
            }
        }
        if (num_prominent == 2) {
            return {"TWOFINGERUP", proms};
        }
    }
    if (num_prominent == 1 || get_last_gesture() == "POINTERUP") {
        if (thumb->is_prominent()) return {"THUMBUP", proms};
        return {"POINTERUP", proms};
    }

    return {Hand::gests[num_prominent], proms};
}

bool Hand::resembles_pinch(std::vector<Finger*> proms) {
    int num_prominent = proms.size();
    bool first_condition = num_prominent == 2 || (get_last_gesture() == "PINCH" || get_last_gesture() == "TWOFINGERUP");
    bool second_condition = thumb->is_prominent();
    std::cout << "RESEMBLES PINCH? " << (first_condition || second_condition) << std::endl;
    return first_condition || second_condition;
}

std::string Hand::verify() {
    auto [new_gesture, proms] = determine_gesture();
    new_gest = new_gesture;

    if (new_gest == get_current_gesture()) {
        verification.clear();
        current_prom = proms;
        return get_current_gesture();
    }

    verification.push_back(new_gest);
    if (verification.size() == 3) {
        std::string gestret;
        if (std::all_of(verification.begin(), verification.end(), [this](const std::string& gest) { return gest == verification[0]; })) {
            gestret = new_gest;
        } else {
            gestret = get_last_gesture();
        }
        new_gest = "";
        verification.clear();
        return gestret;
    }

    current_prom = proms;
    return get_current_gesture();
}

bool Hand::shows_palm() {
    if(true_handtype() == Hand::TYPE::RIGHT) {
        return points[FingerParts::INDEX_FINGER_MCP][1] > points[FingerParts::PINKY_MCP][1];
    } else {
        return points[FingerParts::INDEX_FINGER_MCP][1] < points[FingerParts::PINKY_MCP][1];
    }
}

void Hand::stabililze_middle_knuckle() {
	double d_average = std::abs((this->middle->get_kdx() + this->middle->get_kdy()) / 2);
	std::cout << "d_average: " << d_average << ", middle kd: [" << this->middle->get_kdx() << ", " << this->middle->get_kdy() << "]" << std::endl;
	if(d_average <= MIDDLE_STABILIZER_THRESHOLD && !this->middle->get_last_knuckle().empty()) {
		//print_landmarks(this->points);
		std::cout << "FINGERPARTS::MIDDLEFINGERMCP[1]: " << this->points[FingerParts::MIDDLE_FINGER_MCP][1] << std::endl;
		std::cout << "MIDDLE->GETLASTKNUCKLE()[1]: " << this->middle->get_last_knuckle()[1] << std::endl;
		this->points[FingerParts::MIDDLE_FINGER_MCP][1] = this->middle->get_last_knuckle()[1];
		this->points[FingerParts::MIDDLE_FINGER_MCP][2] = this->middle->get_last_knuckle()[2];
		std::cout << "stabilized middle knuckle." << std::endl;
	}
	std::cout << "end of stabilize method." << std::endl;
}

bool Hand::predict(float comrP[2], int hpts[21][3]) {
    if(prediction_no++ >= 10) 
        return false;
        
    float sumx = 0, sumy = 0;
    
    double thumb_tdx, index_tdx, middle_tdx, ring_tdx, pinky_tdx;
    double thumb_tdy, index_tdy, middle_tdy, ring_tdy, pinky_tdy;
    double thumb_kdx, index_kdx, middle_kdx, ring_kdx, pinky_kdx;
    double thumb_kdy, index_kdy, middle_kdy, ring_kdy, pinky_kdy;
    thumb_tdx = thumb->get_tdx(), index_tdx = index->get_tdx(), middle_tdx = middle->get_tdx();
    ring_tdx = ring->get_tdx(), pinky_tdx = pinky->get_tdx();
    
    thumb_tdy = thumb->get_tdy(), index_tdy = index->get_tdy(), middle_tdy = middle->get_tdy();
    ring_tdy = ring->get_tdy(), pinky_tdy = pinky->get_tdy();
    
    thumb_kdx = thumb->get_kdx(), index_kdx = index->get_kdx(), middle_kdx = middle->get_kdx();
    ring_kdx = ring->get_kdx(), pinky_kdx = pinky->get_kdx();
    
    thumb_kdy = thumb->get_kdy(), index_kdy = index->get_kdy(), middle_kdy = middle->get_kdy();
    ring_kdy = ring->get_kdy(), pinky_kdy = pinky->get_kdy();
    
    double avg_dx, avg_dy;
    avg_dx = (thumb_tdx + index_tdx + middle_tdx + ring_tdx + pinky_tdx
                + thumb_kdx + index_kdx + middle_kdx + ring_kdx + pinky_kdx) / 10;
    avg_dy = (thumb_tdy + index_tdy + middle_tdy + ring_tdy + pinky_tdy
                + thumb_kdy + index_kdy + middle_kdy + ring_kdy + pinky_kdy) / 10;
    
    float mod_rate = (PREDICTION_LIMIT - prediction_no) * PREDICTION_RATE;
    
    // predict thumb
    // (points[FingerParts::THUMB_CMC], points[FingerParts::THUMB_MCP], points[FingerParts::THUMB_IP], points[FingerParts::THUMB_TIP], this);
    hpts[FingerParts::THUMB_CMC][1] = points[FingerParts::THUMB_CMC][1] * (thumb_kdx * mod_rate);
    hpts[FingerParts::THUMB_CMC][2] = points[FingerParts::THUMB_CMC][2] * (thumb_kdy * mod_rate);
    
    hpts[FingerParts::THUMB_MCP][1] = points[FingerParts::THUMB_MCP][1] * (thumb_kdx * mod_rate);
    hpts[FingerParts::THUMB_MCP][2] = points[FingerParts::THUMB_MCP][2] * (thumb_kdy * mod_rate);
    
    hpts[FingerParts::THUMB_IP][1] = points[FingerParts::THUMB_IP][1] * (thumb_tdx * mod_rate);
    hpts[FingerParts::THUMB_IP][2] = points[FingerParts::THUMB_IP][2] * (thumb_tdy * mod_rate);
    
    hpts[FingerParts::THUMB_TIP][1] = points[FingerParts::THUMB_TIP][1] * (thumb_tdx * mod_rate);
    hpts[FingerParts::THUMB_TIP][2] = points[FingerParts::THUMB_TIP][2] * (thumb_tdy * mod_rate);
    
    sumx += hpts[FingerParts::THUMB_CMC][1] + hpts[FingerParts::THUMB_MCP][1] + hpts[FingerParts::THUMB_IP][1] + hpts[FingerParts::THUMB_TIP][1];
    sumy += hpts[FingerParts::THUMB_CMC][2] + hpts[FingerParts::THUMB_MCP][2] + hpts[FingerParts::THUMB_IP][2] + hpts[FingerParts::THUMB_TIP][2];
    
    // predict index
    // (points[FingerParts::INDEX_FINGER_MCP], points[FingerParts::INDEX_FINGER_PIP], points[FingerParts::INDEX_FINGER_DIP], points[FingerParts::INDEX_FINGER_TIP]);
    hpts[FingerParts::INDEX_FINGER_MCP][1] = points[FingerParts::INDEX_FINGER_MCP][1] * (index_kdx * mod_rate);
    hpts[FingerParts::INDEX_FINGER_MCP][2] = points[FingerParts::INDEX_FINGER_MCP][2] * (index_kdy * mod_rate);
    
    hpts[FingerParts::INDEX_FINGER_PIP][1] = points[FingerParts::INDEX_FINGER_PIP][1] * (index_kdx * mod_rate);
    hpts[FingerParts::INDEX_FINGER_PIP][2] = points[FingerParts::INDEX_FINGER_PIP][2] * (index_kdy * mod_rate);
    
    hpts[FingerParts::INDEX_FINGER_DIP][1] = points[FingerParts::INDEX_FINGER_DIP][1] * (index_tdx * mod_rate);
    hpts[FingerParts::INDEX_FINGER_DIP][2] = points[FingerParts::INDEX_FINGER_DIP][2] * (index_tdy * mod_rate);
    
    hpts[FingerParts::INDEX_FINGER_TIP][1] = points[FingerParts::INDEX_FINGER_TIP][1] * (index_tdx * mod_rate);
    hpts[FingerParts::INDEX_FINGER_TIP][2] = points[FingerParts::INDEX_FINGER_TIP][2] * (index_tdy * mod_rate);
    
    sumx += hpts[FingerParts::INDEX_FINGER_MCP][1] + hpts[FingerParts::INDEX_FINGER_PIP][1] + hpts[FingerParts::INDEX_FINGER_DIP][1] + hpts[FingerParts::INDEX_FINGER_TIP][1];
    sumy += hpts[FingerParts::INDEX_FINGER_MCP][2] + hpts[FingerParts::INDEX_FINGER_PIP][2] + hpts[FingerParts::INDEX_FINGER_DIP][2] + hpts[FingerParts::INDEX_FINGER_TIP][2];
    
    // predict middle
    // (points[FingerParts::MIDDLE_FINGER_MCP], points[FingerParts::MIDDLE_FINGER_PIP], points[FingerParts::MIDDLE_FINGER_DIP], points[FingerParts::MIDDLE_FINGER_TIP]);
    hpts[FingerParts::MIDDLE_FINGER_MCP][1] = points[FingerParts::MIDDLE_FINGER_MCP][1] * (middle_kdx * mod_rate);
    hpts[FingerParts::MIDDLE_FINGER_MCP][2] = points[FingerParts::MIDDLE_FINGER_MCP][2] * (middle_kdy * mod_rate);
    
    hpts[FingerParts::MIDDLE_FINGER_PIP][1] = points[FingerParts::MIDDLE_FINGER_PIP][1] * (middle_kdx * mod_rate);
    hpts[FingerParts::MIDDLE_FINGER_PIP][2] = points[FingerParts::MIDDLE_FINGER_PIP][2] * (middle_kdy * mod_rate);
    
    hpts[FingerParts::MIDDLE_FINGER_DIP][1] = points[FingerParts::MIDDLE_FINGER_DIP][1] * (middle_tdx * mod_rate);
    hpts[FingerParts::MIDDLE_FINGER_DIP][2] = points[FingerParts::MIDDLE_FINGER_DIP][2] * (middle_tdy * mod_rate);
    
    hpts[FingerParts::MIDDLE_FINGER_TIP][1] = points[FingerParts::MIDDLE_FINGER_TIP][1] * (middle_tdx * mod_rate);
    hpts[FingerParts::MIDDLE_FINGER_TIP][2] = points[FingerParts::MIDDLE_FINGER_TIP][2] * (middle_tdy * mod_rate);
    
    sumx += hpts[FingerParts::MIDDLE_FINGER_MCP][1] + hpts[FingerParts::MIDDLE_FINGER_PIP][1] + hpts[FingerParts::MIDDLE_FINGER_DIP][1] + hpts[FingerParts::MIDDLE_FINGER_TIP][1];
    sumy += hpts[FingerParts::MIDDLE_FINGER_MCP][2] + hpts[FingerParts::MIDDLE_FINGER_PIP][2] + hpts[FingerParts::MIDDLE_FINGER_DIP][2] + hpts[FingerParts::MIDDLE_FINGER_TIP][2];
    
    // predicts ring
    // (points[FingerParts::RING_FINGER_MCP], points[FingerParts::RING_FINGER_PIP], points[FingerParts::RING_FINGER_DIP], points[FingerParts::RING_FINGER_TIP]);
    hpts[FingerParts::RING_FINGER_MCP][1] = points[FingerParts::RING_FINGER_MCP][1] * (ring_kdx * mod_rate);
    hpts[FingerParts::RING_FINGER_MCP][2] = points[FingerParts::RING_FINGER_MCP][2] * (ring_kdy * mod_rate);
    
    hpts[FingerParts::RING_FINGER_PIP][1] = points[FingerParts::RING_FINGER_PIP][1] * (ring_kdx * mod_rate);
    hpts[FingerParts::RING_FINGER_PIP][2] = points[FingerParts::RING_FINGER_PIP][2] * (ring_kdy * mod_rate);
    
    hpts[FingerParts::RING_FINGER_DIP][1] = points[FingerParts::RING_FINGER_DIP][1] * (ring_tdx * mod_rate);
    hpts[FingerParts::RING_FINGER_DIP][2] = points[FingerParts::RING_FINGER_DIP][2] * (ring_tdy * mod_rate);
    
    hpts[FingerParts::RING_FINGER_TIP][1] = points[FingerParts::RING_FINGER_TIP][1] * (ring_tdx * mod_rate);
    hpts[FingerParts::RING_FINGER_TIP][2] = points[FingerParts::RING_FINGER_TIP][2] * (ring_tdy * mod_rate);
    
    sumx += hpts[FingerParts::RING_FINGER_MCP][1] + hpts[FingerParts::RING_FINGER_PIP][1] + hpts[FingerParts::RING_FINGER_DIP][1] + hpts[FingerParts::RING_FINGER_TIP][1];
    sumy += hpts[FingerParts::RING_FINGER_MCP][2] + hpts[FingerParts::RING_FINGER_PIP][2] + hpts[FingerParts::RING_FINGER_DIP][2] + hpts[FingerParts::RING_FINGER_TIP][2];
    
    // predicts pinky
    // (points[FingerParts::PINKY_MCP], points[FingerParts::PINKY_PIP], points[FingerParts::PINKY_DIP], points[FingerParts::PINKY_TIP]);
    hpts[FingerParts::PINKY_MCP][1] = points[FingerParts::PINKY_MCP][1] * (pinky_kdx * mod_rate);
    hpts[FingerParts::PINKY_MCP][2] = points[FingerParts::PINKY_MCP][2] * (pinky_kdy * mod_rate);
    
    hpts[FingerParts::PINKY_PIP][1] = points[FingerParts::PINKY_PIP][1] * (pinky_kdx * mod_rate);
    hpts[FingerParts::PINKY_PIP][2] = points[FingerParts::PINKY_PIP][2] * (pinky_kdy * mod_rate);
    
    hpts[FingerParts::PINKY_DIP][1] = points[FingerParts::PINKY_DIP][1] * (pinky_tdx * mod_rate);
    hpts[FingerParts::PINKY_DIP][2] = points[FingerParts::PINKY_DIP][2] * (pinky_tdy * mod_rate);
    
    hpts[FingerParts::PINKY_TIP][1] = points[FingerParts::PINKY_TIP][1] * (pinky_tdx * mod_rate);
    hpts[FingerParts::PINKY_TIP][2] = points[FingerParts::PINKY_TIP][2] * (pinky_tdy * mod_rate);
    
    sumx += hpts[FingerParts::PINKY_MCP][1] + hpts[FingerParts::PINKY_PIP][1] + hpts[FingerParts::PINKY_DIP][1] + hpts[FingerParts::PINKY_TIP][1];
    sumy += hpts[FingerParts::PINKY_MCP][2] + hpts[FingerParts::PINKY_PIP][2] + hpts[FingerParts::PINKY_DIP][2] + hpts[FingerParts::PINKY_TIP][2];
    
    // predicts wrist
    hpts[FingerParts::WRIST][1] = points[FingerParts::WRIST][1] * (avg_dx * mod_rate);
    hpts[FingerParts::WRIST][2] = points[FingerParts::WRIST][2] * (avg_dy * mod_rate);
    
    sumx += hpts[FingerParts::WRIST][1];
    sumy += hpts[FingerParts::WRIST][2];
    
    comrP[0] = sumx / 21;
    comrP[1] = sumy / 21;    
    
    return true;    
}

Hand::TYPE Hand::true_handtype() {
    return this->true_hand;
}

int Hand::num_prominent_fingers() {
    return current_prom.size();
}

bool Hand::is_making_gesture() {
    return current_gest != "";
}

std::string Hand::get_current_gesture() {
    return current_gest;
}

std::string Hand::get_last_gesture() {
    return last_gest;
}

std::vector<Finger*> Hand::get_prominent_fingers() {
    return current_prom;
}

Thumb* Hand::get_thumb() {
    return thumb.get();
}

Index* Hand::get_index() {
    return index.get();
}

Middle* Hand::get_middle() {
    return middle.get();
}

Ring* Hand::get_ring() {
    return ring.get();
}

Pinky* Hand::get_pinky() {
    return pinky.get();
}

double Hand::get_x_inches() {
	return x_inches;
}

double Hand::get_y_inches() {
	return y_inches;
}

double Hand::get_depth() {
    return depth;
}

double Hand::get_dz() {
    return dz;
}

double Hand::get_dy() {
	return dy;
}

double Hand::get_dx() {
	return dx;
}

double Hand::get_pitch() {
	return pitch;
}

double Hand::get_yaw() {
	return yaw;
}

double Hand::get_roll() {
	return roll;
}

double Hand::distanceh(Finger* thumb, Finger* finger2) {
    if (thumb->is_prominent() && finger2->is_prominent()) {
        auto tip1 = thumb->get_tip();
        auto tip2 = finger2->get_tip();
        if (tip1.size() < 2 || tip2.size() < 2) return 100.0;

        double t1x = tip1[1], t1y = tip1[2], t2x = tip2[1], t2y = tip2[2];
        return std::sqrt((t2x - t1x) * (t2x - t1x) + (t2y - t1y) * (t2y - t1y));
    }
    return 100.0;
}

