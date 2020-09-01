#!/bin/bash
# Install i386 dependencies
sudo apt update
sudo apt install gcc-multilib g++-multilib
sudo apt install lib32readline6-dev libreadline-dev:i386 libedit-dev:i386

# Compile and Install Custom version of Lemon
cd scripts/lemon
sudo chmod +x ./build.sh
./build.sh
sudo cp ./lemon /usr/bin/
echo "Installed lemon into /usr/bin"