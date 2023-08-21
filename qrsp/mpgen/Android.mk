# Enable RTIC MPGen. Below ENV will trigger RTIC self configuration during the kernel build
# Starting Mar 2018 MPGen has it's own component; override older MPGen location with current
#unexport RTIC_MPGEN
#current_makefile_dir:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
#RTIC_MPGEN := python ${current_makefile_dir}/mpgen.py
#export RTIC_MPGEN

