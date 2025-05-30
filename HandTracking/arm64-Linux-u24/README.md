Hand Tracking
ARM64 Linux (Ubuntu 24.02 LTS)

Disclaimers:
	1) The XR Starter Kit is only compatible with 2 cameras (including a binocular camera. Specify this in `camera_info.json` under the `binocular` key).

How to use:

Camera Calibration:
	* Refer to `../README.md`

Hand Tracking:
	1) Open a terminal within this directory and run the command `bash install.sh`
	2) If `install.sh` ran without any stated errors, you can enter the directory of your choice:
		`./standalone/` is the directory with a program to run on a standalone device, without any information messaging to another computer to handle the computations.
		`./assisted/` is the directory with a program that uses two (2) computers, one as a host, one as the primary computation device.
	3) Enter the directory of your choice, and run the command `./xrhands`

`./standalone/`:
	1) This directory holds a program to run on a standalone device. All that needs to be done in this scenario is to run the command `./xrhands`, and you're set.

`./assisted/`:
	1) This directory holds a program that uses two (2) computers. You will need one computer to host the `./host/` directory's program, and one computer to host the `./computation/` directory's program.
	2) The host device will be the device with the cameras connected to it, while the computational device will be the device handling the hand detection and mapping, and will send the information to the host.
