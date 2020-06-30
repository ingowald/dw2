#!bin/bash

mpirun -n 8 -ppn 1 --hosts sdvis1,sdvis2,sdvis3,sdvis4,sdvis5,sdvis6,sdvis7,sdvis8 \
    ./dw2_testImages /home/sci/mengjiao/Desktop/DisplayWall/dw2/results/ospray-landingGear0.png \
    -hostname 155.98.19.61 -port 2903 -w 23040 -h 5760 -tile-size 256 