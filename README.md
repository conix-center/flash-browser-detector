# FLASH

The FLASH detector detects modulated LEDs to anchor AR content as described [here](http://users.ece.cmu.edu/~agr/resources/publications/ipsn_20_glitter.pdf).
This repository will lead to a more lightweight implementation, able to run in a browser, using WebAssembly.

Based on [apriltag](https://github.com/AprilRobotics/apriltag).

# Building

1. Make sure you have **opencv** installed if you want to compile the opencv examples.
2. Make sure you have **emsdk** and **npm** installed if you want to compile to wasm.
3. Clone this repo (with ```--recurse-submodules```).
4. Compile with npm:
```
npm run build
```
