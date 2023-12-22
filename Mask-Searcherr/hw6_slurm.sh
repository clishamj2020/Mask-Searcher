#!/bin/bash

# Lines beginning with # are comments Only lines beginning #SBATCH are
# processed by slurm

#SBATCH --account=PMIU0184
#SBATCH --job-name=ex18
#SBATCH --time=00:10:00
#SBATCH --mem=1GB
#SBATCH --nodes=2
#SBATCH --tasks-per-node=1

# Run the program 5 times to get measure consistent timings
/usr/bin/time -v ./homework1 images/MiamiMarcumCenter.png images/WindowPane_mask.png result.png true 50 64
perf stat ./homework1 images/MiamiMarcumCenter.png images/WindowPane_mask.png result.png true 50 64
perf record -F 20 --call-graph dwarf -o "perf_job.data" ./homework1 images/MiamiMarcumCenter.png images/WindowPane_mask.png result.png true 50 64
#end of script