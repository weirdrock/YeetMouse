#!/bin/bash
# Uninstall the driver
#sudo dkms remove -m yeetmouse-driver -v 0.9.2 # Newer dkms versions
sudo dkms remove yeetmouse-driver/0.9.2 --all # Older (for Ubuntu <= 20.04), but should work now too
sudo make remove_dkms
sudo rmmod yeetmouse
