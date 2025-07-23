#!/bin/bash
shopt -s expand_aliases
alias arduino-cli=~/bin/arduino-cli/arduino-cli

arduino-cli compile --fqbn arduino:avr:mega .
if [ $? -eq 0 ]; then
arduino-cli upload -p /dev/ttyUSB0 --fqbn arduino:avr:mega --discovery-timeout 10s --verbose .
fi
