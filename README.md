# LIBSMR
A simple C library for reading electrophysiology data files in the [Spike2](http://ced.co.uk/products/spike2)® (CED, Cambridge, UK) SMR format.

## Contents
* `julia/`: julia interface to the library
* `matlab/`: matlab/mex based interface
* `test/`: old debugging / testing utilities that likely do not work
* `smr2mda.c`: program for converting channels from a SMR file to the MountainSort MDA format (for documentation see source or compile and call with `smr2mda -h`)

## Build
* See `Makefile`, currently OOB building is only supported for Linux

## Author:
Scottie Alexander
