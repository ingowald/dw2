#!bin/bash

useHeadNode="$1"
echo "$useHeadNode"

if [[ "$useHeadNode" == "nhn" ]]; then

    mpirun -n 8 -ppn 1 -hosts sdvis1,sdvis2,sdvis3,sdvis4,sdvis5,sdvis6,sdvis7,sdvis8 \
    ./dw2_testOSPRay_impi /home/sci/feng/Desktop/ws/data/NASA_LandingGear_AMR/cb_yvolocity.osp \
    -iso 2400.0 -object /home/sci/feng/Desktop/ws/data/NASA_LandingGear_AMR/landingGear.obj \
    -vp 16.1 15.4 0.27 -vu 0 0 -1 -vi 16.4 15.8 0.33 -sun 0.337 0.416 -0.605 -dis 0.554 1.0 -0.211 \
    -scale 1 1 1 -translate 15.995 16 0.1 -valueRange 98280 99280 \
    -hostname 155.98.19.223 -port 2903 --osp:mpi
else

    mpirun -n 8 -ppn 1 -hosts sdvis1,sdvis2,sdvis3,sdvis4,sdvis5,sdvis6,sdvis7,sdvis8 \
    ./dw2_testOSPRay_impi /home/sci/feng/Desktop/ws/data/NASA_LandingGear_AMR/cb_yvolocity.osp \
    -iso 2400.0 -object /home/sci/feng/Desktop/ws/data/NASA_LandingGear_AMR/landingGear.obj \
    -vp 16.1 15.4 0.27 -vu 0 0 -1 -vi 16.4 15.8 0.33 -sun 0.337 0.416 -0.605 -dis 0.554 1.0 -0.211 \
    -scale 1 1 1 -translate 15.995 16 0.1 -valueRange 98280 99280 \
    -hostname 155.98.19.14 -port 2903 --osp:mpi
fi
