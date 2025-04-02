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
    PyRun_SimpleString(("import sys\nsys.path.append('" + me + "')").c_str());
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





struct Hands PythonManager::sendf(cv::Mat frame) {
    std::cout << "INSIDE SENDF" << std::endl;

    npy_intp dimensions[3] = {frame.rows, frame.cols, frame.channels()};
    PyObject* pargs = PyArray_SimpleNewFromData(3, dimensions, NPY_UINT8, frame.data);
    if (!pargs) {
        std::cerr << "Failed to create NumPy array from data." << std::endl;
        return false;
    }

    PyObject* pval = nullptr;
    try {
		//cv::imshow("BEFORE", frame);
		//cv::waitKey(1);
		//std::cout << "CALLING PROCESS IMAGE NOW..." << std::endl;
        pval = call_function("process_image", pargs);
        //std::cout << "FUNCTION CALLED, PVAL ASSIGNED" << std::endl;
        Py_DECREF(pargs);
        //std::cout << "PARGS DEREFERENCED!" << std::endl;

        if (pval) {
			//std::cout << "IN PVAL" << std::endl;
            bool voidall = true;
            PyObject* repr = PyObject_Str(pval);
            const char* str = PyUnicode_AsUTF8(repr);

            try {
                nlohmann::json j = nlohmann::json::parse(str);
                if(j.empty())
					return false;
                std::cout << j << std::endl;

                float comrL[2];  // [sumLx, sumLy]
                float comrR[2];  // [sumRx, sumRy]
                std::vector<std::vector<int>> lmlistL(21, std::vector<int>(4));	// lmlistL
                std::vector<std::vector<int>> lmlistR(21, std::vector<int>(4));	// lmlistR

                comrL[0] = j[0][0].get<float>();
                comrL[1] = j[0][1].get<float>();

                comrR[0] = j[1][0].get<float>();
                comrR[1] = j[1][1].get<float>();

                for (int i = 0; i < 21; ++i) {
                    if (i < j[2].size() && j[2][i].size() >= 3) {
                        lmlistL[i][0] = j[2][i][0].get<int>();
                        lmlistL[i][1] = j[2][i][1].get<int>();
                        lmlistL[i][2] = j[2][i][2].get<int>();
                        lmlistL[i][3] = j[2][i][3].get<int>();
                    }

                    if (i < j[3].size() && j[3][i].size() >= 3) {
                        lmlistR[i][0] = j[3][i][0].get<int>();
                        lmlistR[i][1] = j[3][i][1].get<int>();
                        lmlistR[i][2] = j[3][i][2].get<int>();
                        lmlistR[i][3] = j[3][i][3].get<int>();
                        voidall = false;
                    }
                }

                bool both = j[4].get<int>();	// tells if both hands are present/detected
                
                std::string leftlabel = j[5].get<std::string>();		// the label associated with the leftmost landmarks
                std::string rightlabel = j[6].get<std::string>();		// the label associated with the rightmost landmarks
                
                // leftlabel and rightlabel string are returned in "Righthand" "Lefthand" format, so [0] is uppercase
                char leftlabelchar = leftlabel != "" ? leftlabel[0] : EMPTY_HAND;
                char rightlabelchar = rightlabel != "" ? rightlabel[0] : EMPTY_HAND;
                
                struct Hands hands = make_hands(comrL, comrR, lmlistL, lmlistR, leftlabelchar, rightlabelchar, both);
                
                return hands;
                
                //std::cout << " VOIDALL: " << voidall << std::endl;
                				
            } catch (const nlohmann::json::parse_error& e) {
                std::cerr << "JSON parsing error: " << e.what() << std::endl;
                Py_DECREF(repr);
                Py_DECREF(pval);
                return false;
            } catch (const std::exception& e) {
                std::cerr << "Exception: " << e.what() << std::endl;
                Py_DECREF(repr);
                Py_DECREF(pval);
                return false;
            }

            Py_DECREF(repr);
            Py_DECREF(pval);
            return true;
        } else {
            std::cout << "Failed to get valid pval!" << std::endl;
            return false;
        }
    } catch (...) {
        Py_XDECREF(pargs);
        Py_XDECREF(pval);
        throw;
    }
}

struct Hands make_hands(
	float comL[COM_LENGTH], float comR[COM_LENGTH,
	int pointsL[POINTS][INFO_PER_POINT], int pointsR[POINTS][INFO_PER_POINT],
	char leftlabel, char rightlabel,
	int both
	) 
{

	struct HandTemplate lefthand;
	struct HandTemplate righthand;

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
	
	struct Hands hands{lefthand, righthand};
	return hands;
	
}


void draw_palm(cv::Mat& frame, const std::vector<std::vector<int>>& shape, const cv::Scalar& color = cv::Scalar(0, 255, 0), int thickness = 2) {
    // Check if the shape vector has at least 3 points (minimum to form a closed polygon)
    if (shape.size() >= 3) {
        // Convert the shape vector to a vector of cv::Points
        std::vector<cv::Point> points;
        for (const auto& point : shape) {
            if (point.size() == 2) { // Check that the inner vector has exactly 2 elements (x, y)
                points.emplace_back(cv::Point(point[0], point[1]));
            } else {
                std::cerr << "Invalid point format in shape. Expected inner vector of size 2." << std::endl;
                return;
            }
        }

        // Draw the polygonal shape on the frame
        const cv::Point* pts = points.data();
        int npts = points.size();
        cv::polylines(frame, &pts, &npts, 1, true, color, thickness);

    } else {
        // If the shape format is incorrect, print an error message
        std::cerr << "Invalid shape format. Expected vector of at least size 3 with inner vectors of size 2." << std::endl;
    }
}
