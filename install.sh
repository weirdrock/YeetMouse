#!/bin/bash

# Install the driver and activate the dkms module
sudo make setup_dkms
sudo dkms install -m yeetmouse-driver -v 0.9.2 # Enter the version you determined from the Makefile earlier in here
sudo modprobe yeetmouse
