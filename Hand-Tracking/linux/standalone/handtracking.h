#ifndef HANDPROC_H
#define HANDPROC_H

#include <opencv2/opencv.hpp>
#include <Python.h>
#include <numpy/arrayobject.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <utility>
#include <array>

#include "hand.h"


#define COM_LENGTH 2
#define POINTS 21
#define INFO_PER_POINT 4




// user methods
struct Hands get_hands(cv::Mat img1, cv::Mat img2);
struct Hands get_hands(cv::Mat img1, cv::Mat img2, std::string yml_path);


// bts functions
void clarify_program_initialization();
void init_program();

struct TemplateHands make_hands(
	float comL[COM_LENGTH], float comR[COM_LENGTH],
	int pointsL[POINTS][INFO_PER_POINT], int pointsR[POINTS][INFO_PER_POINT],
	char leftlabel, char rightlabel,
	int both
	);


int init_numpy();

class PythonManager {

public:
	PythonManager(const std::string& modname);
	~PythonManager();
	
	PyObject* call_function(const std::string& name, PyObject* args);
	void finalize();
	struct TemplateHands sendf(cv::Mat frame);
	
};

void draw_palm(cv::Mat& frame, const std::vector<std::vector<int>>& shape);

#endif
