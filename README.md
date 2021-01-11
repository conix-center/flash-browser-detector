# Glitter Detector

The glitter detector should detect modulated LEDs to anchor AR content as described [here](http://users.ece.cmu.edu/~agr/resources/publications/ipsn_20_glitter.pdf).
This repository should lead to a more lightweight implementation, able to run in a browser, using WebAssembly.

Based on [apriltag](https://github.com/AprilRobotics/apriltag).

# Building

1. Make sure you have **opencv** installed if you want to compile the opencv examples.
1. Make sure you have **emsdk** installed if you want to compile to wasm.
3. Clone this repo (with ```--recurse-submodules```).
4. Compile with make:
```
make
```
