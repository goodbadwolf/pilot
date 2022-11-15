#!/bin/bash

# Clear screen
clear;

# Clear scrollback
printf '\e[3J';

DEVICE=serial
NUM_JOBS=`nproc`
NUM_JOBS=`expr $NUM_JOBS - 3`
NUM_PROCS=4
LOG_LEVEL="WARN"
PRESET_NAME="sphere_32"

function build
{
    echo "Building using jobs = $NUM_JOBS"
    make -j$NUM_JOBS Beams
}

function get_host_fragment
{
    MYHOST=`hostname`
    if [[ $MYHOST = "manishma-desk" ]]; then
        local fragmemt=""
        echo $fragment
    elif [[ $MYHOST = "alaska"  ||  $MYHOST = "cn1" ]]; then
        local fragment="--host cn1,cn2,cn3,cn4"
        echo $fragment
    fi
}

function run
{
    echo "Running with $NUM_PROCS mpi processes, on device $DEVICE"
    HOSTS_FRAGMENT=$(get_host_fragment)
    mpirun -n $NUM_PROCS --bind-to none $HOSTS_FRAGMENT ./examples/beams/Beams --vtkm-device=$DEVICE --vtkm-log-level=$LOG_LEVEL --data-dir=beams_data --preset=$PRESET_NAME
}

while getopts ":d:j:p:l:s:" option; do
    case $option in
        d) DEVICE=$OPTARG;;
        j) NUM_JOBS=$OPTARG;;
        p) NUM_PROCS=$OPTARG;;
        l) LOG_LEVEL=$OPTARG;;
        s) PRESET_NAME=$OPTARG;;
        \?) echo "Invalid option"
        exit;;
    esac
done

build && run
