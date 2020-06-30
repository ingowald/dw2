DisplayWall 2
=============
DisplayWall 2 is a light-weight framework for driving large tiled display wall. 

Building from Source
====================

Dependencies:

-   libjpeg-turbo8-dev

<!-- -   [OSPRay](https://github.com/ospray/ospray) library is needed if BUILD_OSPRAY_TEST_RENDER is ON -->

<span style="background-color:lightblue">Compiling and Installing Client Library</span>
---------------------------------------
Client library is a stand alone library for communicating with the display wall. Users can compile, install the library and link it to any renderer.

- Create a build directory
```cpp
    mkdir dw2/build
    cd dw2/build
```
- Open the CMake configuration dialog
```cpp
    ccmake .. 
```
- Make sure to enable BUILD_CLIENT. And USE_TURBO_JEPG is recommended to enable in order to compress tile images. Then type 'c'onfigure and 'g'enerate. Then build it use 
```cpp
    make -jn install
```

<span style="background-color:lightblue">Finding the client library install with CMake</span>
---------------------------------------------
Find the libray with CMake's `find_package()` function.
```cpp 
find_package(dw2_common REQUIRED)
find_package(dw2_client REQUIRED)
target_link_library(${target} 
                    dw2_common::dw2_common
                    dw2_client::dw2_client)
```

<span style="background-color:lightblue">Compiling Display Wall Service</span>
------------------------------
Display Service is used for driving X11 windows on display wall, letting renderer clients connect to and receiving tile images from clients. 

Follow the instruction above and enable BUILD_SERVICE.

<span style="background-color:lightblue">Compiling Test Renderers</span>
------------------------
Some examples of test renderer can be found in folder /test. Currently, there are 2 test renderers.

- Basic renderer: testFrameRender
Compile with enable BUILD_TEST. `dw2_testFrameRender` is a simple renderer drawing continous colors to pixels. 

- OSPRay renderer: testOSPRay
Compile with enable BUILD_TEST_OSPRAY. `dw2_testOSPRay` is an example of using OSPRay. Currently, it renders spheres at random positions. 
    - In order to run ospray example, compile and install dw2_common and dw2_client library first(following the instruction of compiling and installing the client library).
    - Clone [ospray](https://github.com/ospray/ospray) and clone [ospray_wall](https://github.com/MengjiaoH/ospray_wall) module under ospray/module directory. Compile and install ospray library with enable ospray_module_wall.
    - Compile display wall code with enable BUILD_TEST_OSPRAY

<!-- TEMP
====

Sample calls:


mm && for f in trinity haskell snert walter shadow ranger; do scp dw2_* $f:/tmp/dw2/; done

 
mpirun -N 1 -host trinity,shadow,snert,haskell,walter,ranger ./dw2_service_dryrun -w 3 -h 2 -ws 1600 4800

mpirun -N 1 -host haskell,walter,ranger,trinity,shadow ./dw2_testFrameRenderer trinity 2903 -->



Running:
========


<span style="background-color:lightblue">Launching Displays</span>
-------------------
## Usage
usage: ./dw2_service [args]*

| Variable                       | Description                       |
|:-------------------------------|:----------------------------------|
| --num-displays/-nd <Nx\> <Ny\>     | num displays (total) in x and y dir |
| --width/-w <numDisplays.x\>     | num displays in x direction   |
| --height/-h <numDisplays.y\>    | num displays in y direction   |
| --window-size/-ws <res_x\> <res_y\>   | window size (in pixels) |
| --[no-]head-node / -[n]hn | use / do not use dedicated head node |
| --max-frames-in-flight/-fif <n\>  | allow up to n frames in flight |
| --bezel-width/-bw <Nx\> <Ny\>   | assume a bezel width (between displays) of Nx and Ny pixels  |
| --displays-per-node/-dpn <N\> <display1\> ...<displayN\>] | This only makes sense if the mpi launch uses N ranks per display node. Specifies that each physical host in the mpi launch will have N physical displays attached to, with each display specified through three values: first, the X display is on (ie, :0, :1, etc), and the, pixel coords (x and y) of the lower-left pixel of that X display, with the i'th rank on any given node running the i'th such display. |

Examples below are how to run on SCI powerwall, which has 9 * 4 monitors with resolution 2560 *1440. 

### service, no head node
```cpp
    mpirun -ppn 1 -n 9 -hosts powerwall01,powerwall02,powerwall03,powerwall04,powerwall05,powerwall06,powerwall07,powerwall08,powerwall09 -genv DISPLAY=:0 ./dw2_service -w 9 -h 1 -nhn -fs
```
### service, w/ head node
```cpp
   mpirun -ppn 1 -n 10 -hosts powerwall00,powerwall01,powerwall02,powerwall03,powerwall04,powerwall05,powerwall06,powerwall07,powerwall08,powerwall09 -genv DISPLAY=:0 ./dw2_service -w 9 -h 1 -hn -fs 
```

### service, no head node, with individual ranks per display (meaning manual window placement and proper bezels applying):

- now have to use 4x9=36 ranks, each node has to appear 4 times:

- have to manulaly specify size of each display (glfw will report size
  of all four displays as one 'monitor', which is wrong if we want to
  drive them individually)

- have to specify position of the four displays on each node with --displays-per-node

- have to specify (global) --num-displays as 9x4, not 9x1

- CAN now specify bezel width, too...

Final command:
```cpp
     mpirun -ppn 1 -n 36 -hosts powerwall01,powerwall02,powerwall03,powerwall04,powerwall05,powerwall06,powerwall07,powerwall08,powerwall09,powerwall01,powerwall02,powerwall03,powerwall04,powerwall05,powerwall06,powerwall07,powerwall08,powerwall09,powerwall01,powerwall02,powerwall03,powerwall04,powerwall05,powerwall06,powerwall07,powerwall08,powerwall09,powerwall01,powerwall02,powerwall03,powerwall04,powerwall05,powerwall06,powerwall07,powerwall08,powerwall09 -genv DISPLAY=:0 ./dw2_service -nhn --num-displays 9 4 --window-size 2560 1440 --displays-per-node 4 0 0 4320 0 0 2800 0 0 1440 0 0 0
```

### service, w/ head node, with individual ranks per display (meaning manual window placement and proper bezels applying):
```cpp
     mpirun -ppn 1 -n 37 -hosts powerwall00,powerwall01,powerwall02,powerwall03,powerwall04,powerwall05,powerwall06,powerwall07,powerwall08,powerwall09,powerwall01,powerwall02,powerwall03,powerwall04,powerwall05,powerwall06,powerwall07,powerwall08,powerwall09,powerwall01,powerwall02,powerwall03,powerwall04,powerwall05,powerwall06,powerwall07,powerwall08,powerwall09,powerwall01,powerwall02,powerwall03,powerwall04,powerwall05,powerwall06,powerwall07,powerwall08,powerwall09 -genv DISPLAY=:0 ./dw2_service -nhn --num-displays 9 4 --window-size 2560 1440 --displays-per-node 4 0 0 4320 0 0 2800 0 0 1440 0 0 0
```

Note we do NOT use `-ppn 4` (and instead explicitly specify each node
four times) because ppn 4 would mean rank 0,1,2,3 would all get mapped
to displaywall01, but those four ranks should drive the first four
logical displays in the lower line of displays, NOT the four displays
in the first column...

<span style="background-color:lightblue">Running testFrameRender</span>
-----------------------

### test app, no head node
```cpp
    mpirun -ppn 1 -n 10 -hosts powerwall00,powerwall01,powerwall02,powerwall03,powerwall04,powerwall05,powerwall06,powerwall07,powerwall08,powerwall09  ./dw2_testFrameRenderer powerwall01 2903
```
### test app, head node
```cpp
    mpirun -ppn 1 -n 10 -hosts powerwall00,powerwall01,powerwall02,powerwall03,powerwall04,powerwall05,powerwall06,powerwall07,powerwall08,powerwall09  ./dw2_testFrameRenderer powerwall00 2903
```

<span style="background-color:lightblue">Running testOSPRay</span>
-----------------------

### test app, no head node
```cpp
    mpirun -ppn 1 -n 10 -hosts powerwall00,powerwall01,powerwall02,powerwall03,powerwall04,powerwall05,powerwall06,powerwall07,powerwall08,powerwall09  ./dw2_testOSPRay powerwall01 2903 --osp:mpi
```
### test app, head node
```cpp
    mpirun -ppn 1 -n 10 -hosts powerwall00,powerwall01,powerwall02,powerwall03,powerwall04,powerwall05,powerwall06,powerwall07,powerwall08,powerwall09  ./dw2_testOSPRay powerwall00 2903 --osp:mpi
```



