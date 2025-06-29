#include <iostream>
#include <iostream>
#include <cmath>
#include <array>
#include <opencv2/opencv.hpp>
#include <pthread.h>

#include "Generics.h"


using Vec3 = std::array<double, 3>;


// constants
#define COM_LENGTH 2
#define POINTS 21
#define INFO_PER_POINT 4

#define XYZ	3
#define SQUARE_POINTS 4
#define FINGERS 5

#define SPACE_X 0
#define SPACE_Y 1
#define SPACE_Z 2

#define RAD2DEG 180 / M_PI



// HAND DECLARATIONS

#define 	WRIST 0
#define     THUMB_CMC 1
#define    	THUMB_MCP 2
#define    	THUMB_IP 3 
#define    	THUMB_TIP 4
#define    	INDEX_MCP 5
#define    	INDEX_PIP 6
#define    	INDEX_DIP 7
#define    	INDEX_TIP 8
#define    	MIDDLE_MCP 9
#define    	MIDDLE_PIP 10
#define   	MIDDLE_DIP 11
#define    	MIDDLE_TIP 12
#define    	RING_MCP 13
#define    	RING_PIP 14
#define    	RING_DIP 15
#define    	RING_TIP 16
#define    	PINKY_MCP 17
#define    	PINKY_PIP 18
#define    	PINKY_DIP 19
#define    	PINKY_TIP 20

#define PROPERLY_INITIALIZED 1
#define UNINITIALIZED 0


enum Gestures {
	FIST = 0,				// zero fingers prominent
	ONE_FINGER,				// one finger prominent
	TWO_FINGERS,			// two fingers prominent
	THREE_FINGERS,			// three fingers prominent
	FOUR_FINGERS,			// four fingers prominent
	HANDUP,					// five fingers prominent
	PINCH					// pinch gesture
};


struct HandTemplate {
	
	// center of mass coordinate
	float com[COM_LENGTH];
	
	// point landmarks for each joint
	int points[POINTS][INFO_PER_POINT];
	
	// right or left (hand)
	char label;
	
	HandTemplate() = default;
	HandTemplate(float c[COM_LENGTH], int p[POINTS][INFO_PER_POINT], char l) {
		std::copy(c, c + COM_LENGTH, com);
		for (int i = 0; i < POINTS; ++i)
			for (int j = 0; j < INFO_PER_POINT; ++j)
				points[i][j] = p[i][j];
		label = l;
	}
	HandTemplate& operator=(const HandTemplate& other) {
		if (this != &other) {
			std::copy(other.com, other.com + COM_LENGTH, com);
			for (int i = 0; i < POINTS; ++i)
				for (int j = 0; j < INFO_PER_POINT; ++j)
					points[i][j] = other.points[i][j];
			label = other.label;
		}
		return *this;
	}
	
};

struct TemplateHands {
	
	int init_status = 0;
	struct HandTemplate lefthand, righthand;

};

struct Hand {
	
	struct HandTemplate hand_template;

	double depth;					// automatically converted to the unit specified in the runner script
	double depths[POINTS];
	double space_position[POINTS][XYZ];		// units away from the main hand camera
	
	enum Gestures gesture;
	
	double pitch, yaw, roll;		// x, y, z rotation IN DEGREES
	double rotation[XYZ];

};


struct Hands {

	struct Hand lefthand, righthand;  

};


extern struct Hands hands;
extern struct Hand lefthand, righthand;
extern bool lefthand_exists, righthand_exists;


bool hand_template_is_valid(const struct HandTemplate ht);

void process_all_hands(struct TemplateHands template_hands[]);
void *process_left_hand(void *args);
void *process_right_hand(void *args);

enum Gestures evaluate_gesture(const struct Hand hand);

void print_hands();
void write_hands();


// hand math

std::array<double, POINTS> hand_depths_left(const struct TemplateHands template_hands_left[]);
std::array<double, POINTS> hand_depths_right(const struct TemplateHands template_hands_right[]);

double depth_of_joint(unsigned int index,
					double cam_distance,
					double focal_length,
					const struct HandTemplate hand_leftcam, const struct HandTemplate hand_rightcam);
					
double depth(double cam_distance, double focal_length, double disparity);

std::array<Vec3, POINTS> backproject_hand_landmarks(const struct Hand hand);

std::array<std::pair<int, int>, SQUARE_POINTS> evaluate_hand_prominence_square(const struct Hand hand);
bool is_point_in_polygon(std::pair<int, int> point, const std::array<std::pair<int, int>, SQUARE_POINTS>& polygon);

Vec3 find_rotation_vector(const struct Hand hand);


double point_distance(std::pair<int, int> p1, std::pair<int, int> p2);


Vec3 subtract(const Vec3 &a, const Vec3 &b);
Vec3 cross(const Vec3 &a, const Vec3 &b);
Vec3 normalize(const Vec3 &v);

