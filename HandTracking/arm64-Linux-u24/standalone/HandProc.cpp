#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

#include <opencv2/opencv.hpp>
#include <Python.h>
#include <numpy/arrayobject.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <typeinfo>
#include <unistd.h>
#include <stdexcept>
#include <cstring>

#include "Generics.h"
#include "HandProc.h"


#define EMPTY_HAND '?'


PyObject* pmod;

int init_numpy() {
	import_array1(-1);
	return 0;
}

PythonManager::PythonManager(const std::string& modname) {
	std::cout << "Initializing Python interpreter..." << std::endl;
	Py_Initialize();
	if (init_numpy() < 0) {
		PyErr_Print();
		throw std::runtime_error("Failed to initialize NumPy C-API");
	}

	std::cout << "Setting up Python module search path..." << std::endl;
	PyRun_SimpleString(("import sys\nsys.path.append('" + my_path + "')").c_str());
	PyObject* pname = PyUnicode_FromString(modname.c_str());
	pmod = PyImport_Import(pname);
	Py_DECREF(pname);

	if (!pmod) {
		PyErr_Print();
		throw std::runtime_error("Couldn't load Python module");
	}
}

PyObject* PythonManager::call_function(const std::string& name, PyObject* args) {
	PyObject* pfunc = PyObject_GetAttrString(pmod, name.c_str());
	if (pfunc && PyCallable_Check(pfunc)) {
		PyObject* pArgsTuple = PyTuple_Pack(1, args);
		if (!pArgsTuple) {
			Py_DECREF(pfunc);
			PyErr_Print();
			return nullptr;
		}

		PyObject* pval = PyObject_CallObject(pfunc, pArgsTuple);
		Py_DECREF(pfunc);
		Py_DECREF(pArgsTuple);
		return pval;
	} else {
		Py_XDECREF(pfunc);
		PyErr_Print();
		return nullptr;
	}
}

void PythonManager::finalize() {
	std::cout << "Finalizing Python interpreter..." << std::endl;
	Py_XDECREF(pmod);
	Py_Finalize();
}

PythonManager::~PythonManager() {
	finalize();
}

struct TemplateHands PythonManager::sendf(cv::Mat frame) {
	npy_intp dimensions[3] = {frame.rows, frame.cols, frame.channels()};
	PyObject* pargs = PyArray_SimpleNewFromData(3, dimensions, NPY_UINT8, frame.data);
	if (!pargs) {
		std::cerr << "Failed to create NumPy array from data." << std::endl;
		return TemplateHands{};
	}

	PyObject* pval = call_function("process_image", pargs);
	Py_DECREF(pargs);

	if (!pval || !PyTuple_Check(pval) || PyTuple_Size(pval) != 7) {
		std::cerr << "Invalid Python return format" << std::endl;
		Py_XDECREF(pval);
		return TemplateHands{};
	}

	PyArrayObject* py_comL = (PyArrayObject*) PyTuple_GetItem(pval, 0);
	PyArrayObject* py_comR = (PyArrayObject*) PyTuple_GetItem(pval, 1);
	PyArrayObject* py_lmlistL = (PyArrayObject*) PyTuple_GetItem(pval, 2);
	PyArrayObject* py_lmlistR = (PyArrayObject*) PyTuple_GetItem(pval, 3);
	int both = PyLong_AsLong(PyTuple_GetItem(pval, 4));
	std::string leftlabel = PyUnicode_AsUTF8(PyTuple_GetItem(pval, 5));
	std::string rightlabel = PyUnicode_AsUTF8(PyTuple_GetItem(pval, 6));

	float* comL = (float*) PyArray_DATA(py_comL);
	float* comR = (float*) PyArray_DATA(py_comR);
	int (*lmlistL)[4] = (int (*)[4]) PyArray_DATA(py_lmlistL);
	int (*lmlistR)[4] = (int (*)[4]) PyArray_DATA(py_lmlistR);

	std::vector<std::vector<int>> _lmlistL(21, std::vector<int>(4));
	std::vector<std::vector<int>> _lmlistR(21, std::vector<int>(4));
	for (int i = 0; i < 21; ++i) {
		for (int j = 0; j < 4; ++j) {
			_lmlistL[i][j] = lmlistL[i][j];
			_lmlistR[i][j] = lmlistR[i][j];
		}
	}

	char leftlabelchar = leftlabel.empty() ? EMPTY_HAND : leftlabel[0];
	char rightlabelchar = rightlabel.empty() ? EMPTY_HAND : rightlabel[0];

	TemplateHands template_hands = make_hands(comL, comR, lmlistL, lmlistR, leftlabelchar, rightlabelchar, both);
	draw_palm(frame, _lmlistR);
	if (both) draw_palm(frame, _lmlistL);

	Py_DECREF(pval);
	return template_hands;
}



struct TemplateHands make_hands(
	float comL[COM_LENGTH], float comR[COM_LENGTH],
	int pointsL[POINTS][INFO_PER_POINT], int pointsR[POINTS][INFO_PER_POINT],
	char leftlabel, char rightlabel,
	int both
	) 
{

	struct HandTemplate lefthand{};
	struct HandTemplate righthand{};

	if(both) {
		
		if(leftlabel == 'L') {
			lefthand = {comL, pointsL, leftlabel};
			righthand = {comR, pointsR, rightlabel};
		} else {
			lefthand = {comR, pointsR, leftlabel};
			righthand = {comL, pointsL, rightlabel};
		}
		
	} else {
	
		if(rightlabel == 'R') {
			righthand = {comR, pointsR, rightlabel};
		} else {
			lefthand = {comR, pointsR, rightlabel};
		}
	
	}
	
	struct TemplateHands template_hands{PROPERLY_INITIALIZED, lefthand, righthand};
	return template_hands;
	
}


void draw_palm(cv::Mat& frame, const std::vector<std::vector<int>>& shape) {
	cv::Scalar color = cv::Scalar(0, 255, 0);
	int thickness = 2;
    // Check if the shape vector has at least 3 points (minimum to form a closed polygon)
    if (shape.size() >= 3) {
        // Convert the shape vector to a vector of cv::Points
        std::vector<cv::Point> points;
        for (const auto& point : shape) {
            cv::circle(frame, cv::Point(point[1], point[2]), 3, cv::Scalar(0, 255, 0), -1);
        }
    } else {
        // If the shape format is incorrect, print an error message
        std::cerr << "Invalid shape format. Expected vector of at least size 3 with inner vectors of size 2." << std::endl;
    }
    
    cv::imshow("frame", frame);
}


