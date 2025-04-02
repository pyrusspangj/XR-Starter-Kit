#include "../Generics.h"
#include "Fingers.h"
#include "Hand.h"
#include <cmath>
#include <chrono>
#include <iostream>

std::pair<std::vector<int>, std::vector<int>> tipstamp = std::make_pair(std::vector<int>(), std::vector<int>());
std::pair<std::vector<int>, std::vector<int>> knucklestamp = std::make_pair(std::vector<int>(), std::vector<int>());

const float Finger::VCC = 0.01875f;     // VioX Conversion Constant from pixels-inches for dx. The unit of the VCC is pixels/inches.
const float Finger::Zref = 6.0f;        // Z reference taken from the VioX camera. 12 inches fits in 640 pixels from 6 inches away.
const float Finger::dvTHRESH = 0.5f;
const int Finger::STEPBACK = 50;        // amount of pixels the click thresholder will set

double tdx = 0, kdx = 0, tdx_px = 0, kdx_px = 0;
double tdy = 0, kdy = 0, tdy_px = 0, kdy_px = 0;

Finger::Finger(std::vector<int> MCP, std::vector<int> PIP, std::vector<int> DIP, std::vector<int> TIP, Hand* HAND)
    : MCP(MCP), PIP(PIP), DIP(DIP), TIP(TIP), respective_hand(HAND) {
        ffilter();
        derive();
}

void Finger::fupdate(std::vector<int> MCP, std::vector<int> PIP, std::vector<int> DIP, std::vector<int> TIP) {
    //std::cout << "IN fupdate FOR " << to_string() << std::endl;
    this->MCP = MCP;
    this->PIP = PIP;
    this->DIP = DIP;
    this->TIP = TIP;
    ffilter();
    derive();
    step_threshold();
    //std::cout << "CLICKED: " << clicked << std::endl;
    //std::cout << "EXITING fupdate FOR " << to_string() << std::endl;
}


void Finger::ffilter() {    
    tipstamp.first = tipstamp.second;
    tipstamp.second = TIP;
    
    knucklestamp.first = knucklestamp.second;
    knucklestamp.second = MCP;
}

void Finger::derive() {
    make_tdx();
    make_tdy();
    make_kdx();
    make_kdy();
}

void Finger::step_threshold() {
    //std::cout << "CHECKING STEP_THRESHOLD!" << std::endl;
    
    if(tdy == 0) {
        threshold_bar = TIP[2] + STEPBACK;    // tip y coordinate + stepback constant
    } else if(tdy > 0) {    // positive velocity, finger is moving DOWN
        if(TIP[2] >= threshold_bar && tdy - kdy > dvTHRESH) {    // if tip-y has touched or passed the threshold bar,
                                                                // and is moving independently from the hand, click
            clicked = true;
            threshold_bar = TIP[2] + STEPBACK;
            return;
        }
    } else {                // negative velocity, finger is moving UP
        threshold_bar = TIP[2] - STEPBACK;    // move threshold bar UP
    }
    
    clicked = false;
}

bool is_point_in_polygon(int x, int y, const std::vector<std::vector<int>>& polygon) {
    size_t n = polygon.size();
    if (n < 3) {  // A valid polygon should have at least 3 points
        std::cerr << "Error: Polygon has less than 3 points in is_point_in_polygon().\n";
        return false;
    }

    bool inside = false;
    for (size_t i = 0, j = n - 1; i < n; j = i++) {
        if (polygon[j].size() < 2 || polygon[i].size() < 2) {
            std::cerr << "Error: Polygon[" << j << "] or Polygon[" << i << "] does not have two coordinates.\n";
            return false;
        }

        int xi = polygon[i][0], yi = polygon[i][1];
        int xj = polygon[j][0], yj = polygon[j][1];

        bool intersect = ((yi > y) != (yj > y)) &&
                         (x < (xj - xi) * (y - yi) / (yj - yi) + xi);
        if (intersect) {
            inside = !inside;
        }
    }

    return inside;
}


bool Finger::is_prominent() {
    if (respective_hand == nullptr) return false;

    const auto& bounding_shape = respective_hand->thesquare; // Assuming this now contains the bounding polygon points

    // Check if DIP and TIP points are inside the bounding polygon
    bool in_polygon_dip = is_point_in_polygon(DIP[1], DIP[2], bounding_shape);
    bool in_polygon_tip = is_point_in_polygon(TIP[1], TIP[2], bounding_shape);

    // Return true if the finger's tip is outside the bounding polygon
    return !in_polygon_tip; // && !in_polygon_dip; // Uncomment to also consider DIP
}

double Finger::length() {
    if (!MCP.empty() && !TIP.empty()) {
        double x1 = MCP[1], y1 = MCP[2];
        double x2 = TIP[1], y2 = TIP[2];
        return std::sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
    }
    return 0.0;
}

std::vector<int> Finger::get_tip() {
    return TIP;
}

std::vector<int> Finger::get_knuckle() {
    return MCP;
}

std::vector<int> Finger::get_last_tip() {
	return this->tipstamp.first;
}

std::vector<int> Finger::get_last_knuckle() {
	return this->knucklestamp.first;
}

int Finger::tcurrent_y() {
    if (!tipstamp.second.empty()) {
        return tipstamp.second[2];
    }
    return -1;
}

int Finger::tprevious_y() {
    if (!tipstamp.first.empty()) {
        return tipstamp.first[2];
    }
    return -1;
}

int Finger::tcurrent_x() {
    if(!tipstamp.second.empty()) {
        return tipstamp.second[1];
    }
    return -1;
}

int Finger::tprevious_x() {
    if(!tipstamp.first.empty()) {
        return tipstamp.first[1];
    }
    return -1;
}

int Finger::kcurrent_y() {
    if(!knucklestamp.second.empty()) {
        return knucklestamp.second[2];
    }
    return -1;
}

int Finger::kprevious_y() {
    if(!knucklestamp.first.empty()) {
        return knucklestamp.first[2];
    }
    return -1;
}

int Finger::kcurrent_x() {
    if(!knucklestamp.second.empty()) {
        return knucklestamp.second[1];
    }
    return -1;
}

int Finger::kprevious_x() {
    if(!knucklestamp.first.empty()) {
        return knucklestamp.first[1];
    }
    return -1;
}

void Finger::make_tdx() {    
    tdx_px = (tcurrent_x() - tprevious_x());// / dt.count();                        // in pixels
    // by dividing by dt.count() (t1 - t0) you are getting pixels/sec, not pixels/iteration
    tdx = tdx_px * Finger::VCC;    // in inches
}

void Finger::make_tdy() {    
    tdy_px = (tcurrent_y() - tprevious_y());// / dt.count();
    tdy = tdy_px * Finger::VCC;    // in inches
}

void Finger::make_kdx() {    
    kdx_px = (kcurrent_x() - kprevious_x());// / dt.count();
    kdx = kdx_px * Finger::VCC;    // in inches
    respective_hand->dx += kdx;
}

void Finger::make_kdy() {    
    kdy_px = (kcurrent_y() - kprevious_y());// / dt.count();
    kdy = kdy_px * Finger::VCC;    // in inches
    respective_hand->dy += kdy;
}

double Finger::get_tdx() {
    return tdx;
}

double Finger::get_tdy() {
    return tdy;
}

double Finger::get_tdx_px() {
	return tdx_px;
}

double Finger::get_tdy_px() {
	return tdy_px;
}

double Finger::get_kdx() {
    return kdx;
}

double Finger::get_kdy() {
    return kdy;
}

double Finger::get_kdx_px() {
	return kdx_px;
}

double Finger::get_kdy_px() {
	return kdy_px;
}

double Finger::get_tip_depth() {
	return tip_depth;
}

bool Finger::is_clicked() {
	return clicked;
}

// Individual fingers
Thumb::Thumb(std::vector<int> MCP, std::vector<int> PIP, std::vector<int> DIP, std::vector<int> TIP, Hand* HAND)
    : Finger(MCP, PIP, DIP, TIP, HAND) {}

std::string Thumb::to_string() const {
    return "THUMB";
}

Index::Index(std::vector<int> MCP, std::vector<int> PIP, std::vector<int> DIP, std::vector<int> TIP, Hand* HAND)
    : Finger(MCP, PIP, DIP, TIP, HAND) {}

std::string Index::to_string() const {
    return "INDEX";
}

Middle::Middle(std::vector<int> MCP, std::vector<int> PIP, std::vector<int> DIP, std::vector<int> TIP, Hand* HAND)
    : Finger(MCP, PIP, DIP, TIP, HAND) {}

std::string Middle::to_string() const {
    return "MIDDLE";
}

Ring::Ring(std::vector<int> MCP, std::vector<int> PIP, std::vector<int> DIP, std::vector<int> TIP, Hand* HAND)
    : Finger(MCP, PIP, DIP, TIP, HAND) {}

std::string Ring::to_string() const {
    return "RING";
}

Pinky::Pinky(std::vector<int> MCP, std::vector<int> PIP, std::vector<int> DIP, std::vector<int> TIP, Hand* HAND)
    : Finger(MCP, PIP, DIP, TIP, HAND) {}

std::string Pinky::to_string() const {
    return "PINKY";
}
