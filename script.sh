#!/bin/bash -l

#$ -l h_rt=72:00:00 # Specify the hard time limit for the job
#$ -N finalproject # Give job a name
#$ -pe omp 1 # Number of cores
#$ -m ea


source /projectnb/ec513/materials/HW2/spec-2017/sourceme
build/X86/gem5.opt configs/example/gem5_library/x86-spec-cpu2017-benchmarks.py --image ../disk-image/spec-2017/spec-2017-image/spec-2017 --partition 1 --benchmark 500.perlbench_r --size test