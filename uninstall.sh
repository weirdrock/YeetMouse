#!/bin/bash
# Unbind all mice from the driver
sudo /usr/lib/udev/leetmouse_manage unbind_all
# Uninstall the driver
sudo dkms remove -m leetmouse-driver -v 0.9.0
sudo make remove_dkms && sudo make udev_uninstall
sudo rmmod leetmouse
