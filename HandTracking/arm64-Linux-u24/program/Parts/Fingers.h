#ifndef FINGERS_H
#define FINGERS_H

#include <vector>
#include <string>

class Hand;

class Finger {
public:
    Finger(std::vector<int> MCP, std::vector<int> PIP, std::vector<int> DIP, std::vector<int> TIP, Hand* HAND);
    void fupdate(std::vector<int> MCP, std::vector<int> PIP, std::vector<int> DIP, std::vector<int> TIP);
    virtual std::string to_string() const = 0;
    
    std::pair<std::vector<int>, std::vector<int>> tipstamp;
    std::pair<std::vector<int>, std::vector<int>> knucklestamp;
    
    double tdx, kdx, tdx_px, kdx_px;
    double tdy, kdy, tdy_px, kdy_px;
    double tip_depth;

    bool is_prominent();
    double length();
    std::vector<int> get_tip();
    std::vector<int> get_knuckle();
    std::vector<int> get_last_tip();
    std::vector<int> get_last_knuckle();
    int tcurrent_y();
    int tprevious_y();
    int tcurrent_x();
    int tprevious_x();
    int kcurrent_y();
    int kprevious_y();
    int kcurrent_x();
    int kprevious_x();
    double get_tdx();
    double get_tdy();
    double get_tdx_px();
    double get_tdy_px();
    double get_kdx();
    double get_kdy();
    double get_kdx_px();
    double get_kdy_px();
    double get_tip_depth();
    bool is_clicked();

    void ffilter();
    void derive(); // derivatives x and y for velocity
    void step_threshold();	// sets the threshold bar for a finger's click threshold
    void percept_depth();

    void make_tdx();
    void make_tdy();
    void make_kdx();
    void make_kdy();

    std::vector<int> MCP, PIP, DIP, TIP;
    Hand* respective_hand;
    double threshold_bar;
    bool clicked;
    
    static const float VCC;			// VioX Conversion Constant from pixels-inches for dx. The unit of the VCC is pixels/inches.
	static const float Zref;		// Z reference taken from the VioX camera. 12 inches fits in 640 pixels from 6 inches away.
	static const float dvTHRESH;	// tdy - kdy should be greater than dvTHRESH for a click to be valid.
    static const int STEPBACK;		// how much the click thresholder will move in addition to your finger

};

class Thumb : public Finger {
public:
    Thumb(std::vector<int> MCP, std::vector<int> PIP, std::vector<int> DIP, std::vector<int> TIP, Hand* HAND);
    std::string to_string() const override;
};

class Index : public Finger {
public:
    Index(std::vector<int> MCP, std::vector<int> PIP, std::vector<int> DIP, std::vector<int> TIP, Hand* HAND);
    std::string to_string() const override;
};

class Middle : public Finger {
public:
    Middle(std::vector<int> MCP, std::vector<int> PIP, std::vector<int> DIP, std::vector<int> TIP, Hand* HAND);
    std::string to_string() const override;
};

class Ring : public Finger {
public:
    Ring(std::vector<int> MCP, std::vector<int> PIP, std::vector<int> DIP, std::vector<int> TIP, Hand* HAND);
    std::string to_string() const override;
};

class Pinky : public Finger {
public:
    Pinky(std::vector<int> MCP, std::vector<int> PIP, std::vector<int> DIP, std::vector<int> TIP, Hand* HAND);
    std::string to_string() const override;
};

#endif // FINGERS_H
