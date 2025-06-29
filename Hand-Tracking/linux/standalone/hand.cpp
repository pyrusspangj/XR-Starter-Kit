#include <limits>
#include <cmath>
#include <cstring>

#include "hand.h"

struct Hands hands;
struct Hand lefthand, righthand;
bool lefthand_exists, righthand_exists;


struct pthread_attr_hand_proccessing_info {
	struct TemplateHands *template_hands;
	struct Hand *hand;
};



bool hand_template_is_valid(const struct HandTemplate ht) {
	return ht.label != 0;
}

void process_all_hands(struct TemplateHands template_hands[]) {

	struct Hand left{template_hands[0].lefthand}, right{template_hands[0].righthand};
	lefthand_exists = righthand_exists = true;
		
	struct pthread_attr_hand_proccessing_info left_processing_info{template_hands, &left};
	struct pthread_attr_hand_proccessing_info right_processing_info{template_hands, &right};
	
	pthread_t lefthand_thread, righthand_thread;
	
	pthread_create(&lefthand_thread, nullptr, process_left_hand, &left_processing_info);
	pthread_create(&righthand_thread, nullptr, process_right_hand, &right_processing_info);
	
	pthread_join(lefthand_thread, nullptr);
	pthread_join(righthand_thread, nullptr);
	
	// assign hand.h hands appropriately (as a global variable for access)
	lefthand = left;
	righthand = right;
	
	hands = Hands{left, right};
	
	print_hands();

}

void *process_left_hand(void *args) {
	
	struct pthread_attr_hand_proccessing_info info = *( (struct pthread_attr_hand_proccessing_info *)args );
	struct TemplateHands template_hands[MAXCAMS];
	std::copy(info.template_hands, info.template_hands + MAXCAMS, template_hands);
	struct Hand *left = info.hand;
	
	// invalid/nonexistent hand
	if(!hand_template_is_valid(template_hands[0].lefthand)) {
		lefthand_exists = false;
		return nullptr;
	}
	
	// extract the hand's information
	std::array<double, POINTS> leftdepths;
	leftdepths = hand_depths_left(template_hands);
		
	double leftdepth;
	leftdepth = leftdepths.at(WRIST);
	
	
	// assign the information to the official hand object
	left->depth = leftdepth;
	std::copy(leftdepths.begin(), leftdepths.end(), left->depths);
	
	
	// calculate position in space by hand object
	std::array<Vec3, POINTS> space_position;
	space_position = backproject_hand_landmarks(*left);
	
	for(int point=0; point<POINTS; point++) {
		for(int coordinate=0; coordinate<XYZ; coordinate++) {
			left->space_position[point][coordinate] = space_position.at(point).at(coordinate);
		}
	}
	
	left->gesture = evaluate_gesture(*left);
	
	Vec3 rotation_vector = find_rotation_vector(*left);
	std::copy(rotation_vector.begin(), rotation_vector.end(), left->rotation);
	
	return nullptr;
}

void *process_right_hand(void *args) {
	
	struct pthread_attr_hand_proccessing_info info = *( (struct pthread_attr_hand_proccessing_info *)args );
	struct TemplateHands template_hands[MAXCAMS];
	std::copy(info.template_hands, info.template_hands + MAXCAMS, template_hands);
	struct Hand *right = info.hand;
	
	// invalid/nonexistent hand
	if(!hand_template_is_valid(template_hands[0].righthand)) {
		righthand_exists = false;
		return nullptr;
	}
	
	// extract the hand's information
	std::array<double, POINTS> rightdepths;
	rightdepths = hand_depths_right(template_hands);
		
	double rightdepth;
	rightdepth = rightdepths.at(WRIST);
	
	
	// assign the information to the official hand object
	right->depth = rightdepth;
	std::copy(rightdepths.begin(), rightdepths.end(), right->depths);
	
	
	// calculate position in space by hand object
	std::array<Vec3, POINTS> space_position;
	space_position = backproject_hand_landmarks(*right);
	
	for(int point=0; point<POINTS; point++) {
		for(int coordinate=0; coordinate<XYZ; coordinate++) {
			right->space_position[point][coordinate] = space_position.at(point).at(coordinate);
		}
	}
	
	right->gesture = evaluate_gesture(*right);
	
	Vec3 rotation_vector = find_rotation_vector(*right);
	std::copy(rotation_vector.begin(), rotation_vector.end(), right->rotation);
	
	return nullptr;
}

enum Gestures evaluate_gesture(const struct Hand hand) {
	
	std::array<std::pair<int, int>, SQUARE_POINTS> handsquare = evaluate_hand_prominence_square(hand);
	bool prominent_fingers[FINGERS];		// 0: thumb, 1: index, 2: middle, 3: ring, 4: pinky
	std::vector<std::pair<int, int>> prominent_points;
	int prominent_num = 0;
	
	int points[POINTS][INFO_PER_POINT];
	std::memcpy(points, hand.hand_template.points, sizeof(points));

	std::pair<int, int> point = {points[THUMB_TIP][1], points[THUMB_TIP][2]};
	
	for(int f=0; f<FINGERS; f++) {
		bool is_tip_in_polygon;
		switch(f) {
			case 0:		// thumb
				point = {points[THUMB_TIP][1], points[THUMB_TIP][2]};
				break;
			
			case 1:		// index
				point = {points[INDEX_TIP][1], points[INDEX_TIP][2]};
				break;
				
			case 2:		// middle
				point = {points[MIDDLE_TIP][1], points[MIDDLE_TIP][2]};
				break;
				
			case 3:		// ring
				point = {points[RING_TIP][1], points[RING_TIP][2]};
				break;
			
			case 4:		// pinky
				point = {points[PINKY_TIP][1], points[PINKY_TIP][2]};
				break;
		}
		
		is_tip_in_polygon = is_point_in_polygon(point, handsquare);
		prominent_fingers[f] = !is_tip_in_polygon;
		prominent_num += (int)(!is_tip_in_polygon);
		
		if(prominent_fingers[f]) prominent_points.push_back(point);
	}
	
	// two prominent fingers and thumb is prominent
	if(prominent_num == 2 && prominent_fingers[0]) {
		if(point_distance(prominent_points[0], prominent_points[1]) <= PINCH_THRESHOLD) {
			return Gestures::PINCH;
		}
	}
	
	return (Gestures)prominent_num;
	
}

void print_hands() {
	std::cout << "Print Hands:" << std::endl;
	
	std::cout << "\nLeft Hand:\t\t\t\t\tRight Hand:" << std::endl;
	std::cout << "Depth: " << lefthand.depth << "\t\t\t\t\t" << righthand.depth << std::endl;
	std::cout << "Space: [" << lefthand.space_position[0][0] << ", " << lefthand.space_position[0][1] << ", " << lefthand.space_position[0][2] << "]\t\t\t";
	std::cout << "[" << righthand.space_position[0][0] << ", " << righthand.space_position[0][1] << ", " << righthand.space_position[0][2] << "]\n" << std::endl;
}

void write_hands() {
	
	nlohmann::json hand_data_json = load_json_file(HAND_DATA);
	
	if(lefthand_exists) {
		hand_data_json["left"] = lefthand.space_position;
	} else {
		hand_data_json["left"] = nullptr;
	}
	
	if(righthand_exists) {
		hand_data_json["right"] = righthand.space_position;
	} else {
		hand_data_json["right"] = nullptr;
	}
	
	write_json_with_lock(HAND_DATA, hand_data_json);
	
}


// hand math

std::array<double, POINTS> hand_depths_left(const struct TemplateHands template_hands_left[]) {
	std::array<double, POINTS> depths{};
	for(int i = 0; i < POINTS; i++) {
		depths[i] = 0;
		for(int j = 0; j < MAXCAMS - 1; j++) {
			depths[i] += depth_of_joint(i, setup["distance"], fx,
				template_hands_left[j].lefthand,
				template_hands_left[j + 1].lefthand);
		}
	}
	return depths;
}

std::array<double, POINTS> hand_depths_right(const struct TemplateHands template_hands_right[]) {
	std::array<double, POINTS> depths{};
	for(int i = 0; i < POINTS; i++) {
		depths[i] = 0;
		for(int j = 0; j < MAXCAMS - 1; j++) {
			depths[i] += depth_of_joint(i, setup["distance"], fx,
				template_hands_right[j].righthand,
				template_hands_right[j + 1].righthand);
		}
	}
	return depths;
}

double depth_of_joint(unsigned int index, double cam_distance, double focal_length,
                      const struct HandTemplate hand_leftcam, const struct HandTemplate hand_rightcam) {
	int point_leftcam[INFO_PER_POINT], point_rightcam[INFO_PER_POINT];
	
	std::memcpy(point_leftcam, hand_leftcam.points[index], sizeof(hand_leftcam.points[index]));
	std::memcpy(point_rightcam, hand_rightcam.points[index], sizeof(hand_rightcam.points[index]));
	
	double disparity = std::abs(point_leftcam[1] - point_rightcam[1]);
	return depth(cam_distance, focal_length, disparity);
}

double depth(double cam_distance, double focal_length, double disparity) {
	return (focal_length * cam_distance) / disparity;
}

std::array<Vec3, POINTS> backproject_hand_landmarks(const struct Hand hand) {
	std::array<Vec3, POINTS> spaces;
	for(int i = 0; i < POINTS; i++) {
		int x = hand.hand_template.points[i][1];
		int y = hand.hand_template.points[i][2];
		double z = hand.depths[i];
		double px = (x - cx) * z / fx;
		double py = (y - cy) * z / fy;
		spaces[i] = {px, py, z};
	}
	
	std::cout << "x: " << hand.hand_template.points[0][1] << ", y: " << hand.hand_template.points[0][2] << std::endl;
	std::cout << "cx: " << cx << ", cy: " << cy << ", fx: " << fx << ", fy: " << fy << std::endl;
	
	return spaces;
}

std::array<std::pair<int, int>, SQUARE_POINTS> evaluate_hand_prominence_square(const struct Hand hand) {
	float BASE_OFFSET = 25;
	int OFFSET = BASE_OFFSET / (hand.depth / 5.5);

	int palm_points[7][INFO_PER_POINT];

	std::memcpy(palm_points[0], hand.hand_template.points[WRIST], sizeof(palm_points[0]));
	std::memcpy(palm_points[1], hand.hand_template.points[THUMB_CMC], sizeof(palm_points[0]));
	std::memcpy(palm_points[2], hand.hand_template.points[THUMB_MCP], sizeof(palm_points[0]));
	std::memcpy(palm_points[3], hand.hand_template.points[INDEX_MCP], sizeof(palm_points[0]));
	std::memcpy(palm_points[4], hand.hand_template.points[MIDDLE_MCP], sizeof(palm_points[0]));
	std::memcpy(palm_points[5], hand.hand_template.points[RING_MCP], sizeof(palm_points[0]));
	std::memcpy(palm_points[6], hand.hand_template.points[PINKY_MCP], sizeof(palm_points[0]));

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

	return {
		std::make_pair(std::max(x_min - OFFSET, 0), std::max(y_min - OFFSET, 0)),
		std::make_pair(std::min(x_max + OFFSET, capture_width), std::max(y_min - OFFSET, 0)),
		std::make_pair(std::min(x_max + OFFSET, capture_width), std::min(y_max + OFFSET, capture_height)),
		std::make_pair(std::max(x_min - OFFSET, 0), std::min(y_max + OFFSET, capture_height))
	};
}

bool is_point_in_polygon(std::pair<int, int> point, const std::array<std::pair<int, int>, SQUARE_POINTS>& polygon) {
	int x = point.first, y = point.second;
	size_t n = polygon.size();
	bool inside = false;
	for (size_t i = 0, j = n - 1; i < n; j = i++) {
		int xi = polygon[i].first, yi = polygon[i].second;
		int xj = polygon[j].first, yj = polygon[j].second;
		bool intersect = ((yi > y) != (yj > y)) &&
			(x < (xj - xi) * (y - yi) / double(yj - yi + 1e-5) + xi);
		if (intersect) inside = !inside;
	}
	return inside;
}

Vec3 find_rotation_vector(const struct Hand hand) {
	double sp[POINTS][XYZ];
	std::memcpy(sp, hand.space_position, sizeof(hand.space_position));
	
	Vec3 wrist = {sp[WRIST][SPACE_X], sp[WRIST][SPACE_Y], sp[WRIST][SPACE_Z]};
	Vec3 index_mcp = {sp[INDEX_MCP][SPACE_X], sp[INDEX_MCP][SPACE_Y], sp[INDEX_MCP][SPACE_Z]};
	Vec3 pinky_mcp = {sp[PINKY_MCP][SPACE_X], sp[PINKY_MCP][SPACE_Y], sp[PINKY_MCP][SPACE_Z]};

	Vec3 x_axis = normalize(subtract(index_mcp, wrist));
	Vec3 pinky_vec = subtract(pinky_mcp, wrist);
	Vec3 z_axis = normalize(cross(x_axis, pinky_vec));
	Vec3 y_axis = normalize(cross(z_axis, x_axis));

	double R[XYZ][XYZ] = {
		{x_axis[0], y_axis[0], z_axis[0]},
		{x_axis[1], y_axis[1], z_axis[1]},
		{x_axis[2], y_axis[2], z_axis[2]}
	};

	double yaw = std::atan2(R[1][0], R[0][0]) * RAD2DEG;
	double pitch = std::atan2(-R[2][0], std::sqrt(R[2][1]*R[2][1] + R[2][2]*R[2][2])) * RAD2DEG;
	double roll = std::atan2(R[2][1], R[2][2]) * RAD2DEG;

			// x    y    z
	return {pitch, yaw, roll};
}

double point_distance(std::pair<int, int> p1, std::pair<int, int> p2) {
	double dx = static_cast<double>(p2.first - p1.first);
	double dy = static_cast<double>(p2.second - p1.second);
	return std::sqrt(dx * dx + dy * dy);
}

Vec3 subtract(const Vec3 &a, const Vec3 &b) {
	return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}

Vec3 cross(const Vec3 &a, const Vec3 &b) {
	return {
		a[1] * b[2] - a[2] * b[1],
		a[2] * b[0] - a[0] * b[2],
		a[0] * b[1] - a[1] * b[0]
	};
}

Vec3 normalize(const Vec3 &v) {
	double len = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	return {v[0]/len, v[1]/len, v[2]/len};
}
