#!/bin/bash

NODES=$1
VERSION=$2

mpirun -n $1 --hostfile $OAR_NODEFILE --mca btl_tcp_if_include bond0 mpi-heat-diffusion/$VERSION
