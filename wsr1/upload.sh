#!/bin/bash
shopt -s expand_aliases
alias arduino-cli=~/bin/arduino-cli/arduino-cli

# default --discovery-timeout in 2024 is 1s, 
# too fast for my machine & the software driven USB interface after reset

# for the micro, if the verbose "waiting for upload port" stalls, press reset on board

arduino-cli compile --fqbn arduino:avr:micro .
arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:avr:micro --discovery-timeout 10s --verbose .

# arduino-cli compile --fqbn arduino:avr:uno .
# arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:avr:uno .

# arduino-cli upload -p /dev/ttyUSB0 --fqbn arduino:avr:uno .
# arduino-cli upload -p /dev/ttyUSB0 --fqbn esp8266:esp8266:thing .

