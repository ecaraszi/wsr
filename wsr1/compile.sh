#!/bin/bash
shopt -s expand_aliases
alias arduino-cli=~/bin/arduino-cli/arduino-cli

arduino-cli compile --warnings all --fqbn arduino:avr:micro .

# arduino-cli compile --fqbn arduino:avr:uno .
# arduino-cli compile --fqbn esp8266:esp8266:thing .
