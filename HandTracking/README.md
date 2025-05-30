Hand Tracking
Version: Alpha 1.0

How to use:

Camera Calibration:
	1) Travel to the `Hand-Tracking/Camera-Calibration` directory. There are two (2) Python programs within this directory: `getImages.py`, and `calibration.py`. 
	2) Run `getImages.py`. You will be prompted with a video feed of your camera, and terminal instructions on how to take a snapshot of a frame.
		--- The next few steps are very important, so read carefully ---
	3) Get a picture of a checkerboard (preferably black and white). It can be digital (on your phone or tablet) or physical (printed paper or actual checkerboard). 
	4) Bring the checkerboard into good view of the camera, and begin taking multiple snapshots of the camera frames as instructed by the terminal.
		Move the checkerboard around the frame with different angles, taking multiple snapshots of multiple positions. You should have around 15-25 pictures taken.
	5) Count the number of rows (how many checkerboard pieces tall the board is)
		Count the number of columns (how many checkerboard pieces wide the board is)
		Measure the length of a single checkerboard piece in millimeters (millimeters = centimeters * 10)
	6) Open the `calibration.py` file to edit. You will see three (3) variables: `CHECKERBOARD_ROWS`, `CHECKERBOARD_COLUMNS`, and `CHECKERBOARD_PIECE_SIZE_IN_MILLIMETERS`
		Set these variables to the information you just counted & measured. 
	7) Run `calibration.py`. You should see some of the images you just took appear with colored lines and circles along the checkerboard pictured. 
		!!! If you receive an output saying "ERROR: No checkerboard could be detected in any of the provided photos.", 
		that means that the checkerboard pictures you took were insufficient & the program could not identify the checkerboard in any of them.
	8) If instead you receive an output saying "SUCCESS! Camera metrics have been calculated.", then your camera has been calibrated.
	9) You will receive an output stating the "total error" of your camera calibration. If this error is greater than `0.10`, restart the process from Step 1.\
	10) All done! Confirm that `guided_calibraiton.yml` exists within the `Camera-Calibration` directory. If so, continue.
	
Hand Tracking:
	1) Enter the directory which aligns with your system:
		`./arm64-Linux-u24/`	->	Supports Linux ARM64 architecture, developed on Ubuntu LTS 24.02
		`./x64-Linux-u24/`		->	Supports Linux x64 architecture, developed on Ubuntu-based (Pop!_OS) LTS 24.02
		`./x64-Windows/`		->	Supports Windows x64 architecture
	2) Follow the instructions in the respective `README.md` file.
