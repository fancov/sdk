#!/bin/bash

sudo make clean 
sudo rm -f monitor.dep
sudo touch monitor.dep
sudo make dep
sudo make all
