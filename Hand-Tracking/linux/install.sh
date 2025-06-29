#!/bin/bash

errors=0
PIP_PACKAGES_INSTALLED=SUCCESS
APT_PACKAGES_INSTALLED=SUCCESS
STANDALONE_COMPILED=SUCCESS
ASSISTED_COMPILED=SUCCESS

PROGRAM_NAME="Hand Tracking - ARM64 Linux (Ubuntu 24.02 LTS)"

echo "Running 'install.sh' for $PROGRAM_NAME"
echo "WARNING: Any errors raised during the execution of this script WILL NOT cause the script to break. All disruptances will be lightly documented at the end of the script's execution."

echo -e "\n1. Installing pip packages..."
pip3 install opencv-python numpy mediapipe
if [[ $? -ne 0 ]]; then
	echo "ERROR in installing the necessary pip packages."
	PIP_PACKAGES_INSTALLED=ERROR
else 
	echo "pip packages successfully installed!"
fi

echo -e "\n2. Installing apt packages..."
sudo apt install build-essential libopencv-dev libgtk-3-dev libboost-all-dev nlohmann-json3-dev python3-dev python3-numpy pkg-config python3-tk -y libgflags-dev libgoogle-glog-dev ocl-icd-opencl-dev
if [[ $? -ne 0 ]]; then
	echo "ERROR in installing the necessary apt packages."
	APT_PACKAGES_INSTALLED=ERROR
else
	echo "apt packages successfully installed!"
fi


echo -e "\n3. Compiling the './standalone/' hand tracking program..."
cd standalone
make
if [[ $? -ne 0 ]]; then
	echo "ERROR compiling the './standalone/' hand tracking program."
	STANDALONE_COMPILED=ERROR
fi
cd ..

#echo -e "\n4. Compiling the './assisted/' hand tracking program..."
#cd assisted
#make
#if [[ $? -ne 0 ]]; then
#	echo "ERROR compiling the './assisted/ hand tracking program."
#	ASSISTED_COMPILED=ERROR
#fi

installation_status="SUCCESSFUL"
for requirement in ${PIP_PACKAGES_INSTALLED} ${APT_PACKAGES_INSTALLED} ${STANDALONE_COMPILED}; do #${ASSISTED_COMPILED}; do
	if [ $requirement == ERROR ]; then
		errors=$((errors + 1))
		installation_status="NOT SUCCESSFUL"
	fi
done

echo -e "\n$PROGRAM_NAME installation was $installation_status"
echo "PIP_PACKAGES_INSTALLED: $PIP_PACKAGES_INSTALLED"
echo "APT_PACKAGES_INSTALLED: $APT_PACKAGES_INSTALLED"
echo "STANDALONE_COMPILED: $STANDALONE_COMPILED"
#echo "ASSISTED_COMPILED: $ASSISTED_COMPILED"

echo ""
if [[ $errors -ne 0 ]]; then
	echo "${errors} errors present. Do not proceed to Step 2 in README.md"
else
	echo "No errors present! Proceed to Step 2 in README.md"
fi
