#!/bin/bash

echo "---- System Information Script ----"

# Display the current user and terminal
echo "Current User: $(whoami)"
echo "Terminal: $TERM"
echo "Shell: $SHELL"

# Display running processes
echo -e "\nCurrent Running Processes:"
ps aux

# Display current date, time, and kernel version
echo -e "\nCurrent Date and Time:"
date
echo -e "\nKernel Version:"
uname -r

# Display kernel dump
echo -e "\nKernel Dump (dmesg output):"
dmesg | tail -n 20   # Limiting output to the last 20 lines for brevity

echo -e "\n---- End of System Information ----"