#!/bin/bash
shopt -s expand_aliases
alias arduino-cli=~/bin/arduino-cli/arduino-cli

arduino-cli compile --warnings all --fqbn arduino:avr:mega .


