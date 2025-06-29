Hand Tracking
Linux (Debian/Ubuntu-based)

Disclaimers:
	1) The XR Starter Kit is only compatible with 2 cameras.

How to use:

Camera Calibration:
	* Refer to `XR-Starter-Kit/Camera-Calibration`

Hand Tracking:
	1) Open a terminal within this directory and run the command `bash install.sh`
	2) If `install.sh` ran without any stated errors, you can enter the program/library directory:
		`./standalone/` provides two useful functions:
		1. Running the `xrhands` executable is a program to run on its own.
		2. The file `handtracking.h` provides the necessary function for use in other programs.
