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

#include "Parts/Hand.h"



#define COM_LENGTH 2
#define POINTS 21
#define INFO_PER_POINT 4

struct HandTemplate {
	
	// center of mass coordinate
	float com[COM_LENGTH];
	
	// point landmarks for each joint
	int points[POINTS][INFO_PER_POINT];
	
	// right or left (hand)
	char label;
	
};

struct Hands {

	struct HandTemplate left;
	struct HandTemplate right;

};

struct Hands make_hands(
	float comL[COM_LENGTH], float comR[COM_LENGTH,
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
	struct Hands sendf(cv::Mat frame);
	
};

#endif
