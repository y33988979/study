#!/bin/sh

echo 1 > /sys/kernel/debug/tracing/events/power/cpu_frequency/enable
echo 1 > /sys/kernel/debug/tracing/events/power/cpu_frequency_limits/enable

echo 1 > /sys/kernel/debug/tracing/tracing_on
cpupower -c 2 frequency-set -g performance
busybox usleep 300
cpupower -c 2 frequency-set -g powersave
busybox usleep 300
echo 0 > /sys/kernel/debug/tracing/tracing_on

cat /sys/kernel/debug/tracing/trace > cpupower.log

