#!/bin/bash

# Use the absolute path to the executable
program="/VMIC/VioX Main Camera/VMC"

a76_cores="4-7"
eff_cores="0-3"

# -20 highest, 19 lowest
niceness=-20

# realtime
chrt_rate=99

# Set CPU frequency governor to performance mode
echo "Setting CPU governor to performance mode"
for cpu in /sys/devices/system/cpu/cpu[0-7]; do
    sudo sh -c "echo performance > $cpu/cpufreq/scaling_governor"
done

# Check if parameters are passed and assign them to variables
param1=${1:-""}

# Run the program with highest priority and on specific cores
sudo sysctl -w kernel.sched_rt_runtime_us=-1
sudo -E chrt -f $chrt_rate nice -n $niceness taskset -c $a76_cores "$program" $param1
