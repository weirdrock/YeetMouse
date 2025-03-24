#!/bin/bash

# Get the installed version of the driver
installed_version=$(dkms status | grep -oP '^([l|y]eetmouse-driver[\/(, )]) ?\K([0-9.]+)')

if [[ ! -z "$installed_version" ]] then
	echo "Driver ($installed_version) already installed, exiting."
	exit 0
fi

# Install the driver and activate the dkms module
sudo make setup_dkms
sudo dkms install -m yeetmouse-driver -v 0.9.2 # Enter the version you determined from the Makefile earlier in here
sudo modprobe yeetmouse
