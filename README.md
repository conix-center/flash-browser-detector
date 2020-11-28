# Glitter Detector

The glitter detector should detect a modulated LEDs to anchor AR content as described [here](http://users.ece.cmu.edu/~agr/resources/publications/ipsn_20_glitter.pdf).
This repository should lead to a more lightweight implementation, able to run in a browser, using WebAssembly.

Based on [apriltag](https://github.com/AprilRobotics/apriltag).

# Quick Setup

1. Make sure you have **opencv** installed if you want to compile the opencv example. **cmake** is required.
2. Clone this repo (with ```--recurse-submodules```).
3. Compile with cmake:
```
./build.sh
```
4. Execute **apriltag_demo**, **apriltag_quads** providing the input image (```-d``` flag for debug output), e.g.:
```bash
./bin/apriltag_quads -d quad_test.jpg
```
* apritag_quads will generate a ```quad_output.ps``` file with the outlines of the quads detected.

5. The **opencv_demo** and **webcam_*** grabs images directly from your webcam.
