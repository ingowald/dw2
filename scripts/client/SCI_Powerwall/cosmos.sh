#!bin/bash

useHeadNode="$1"
echo "$useHeadNode"

if [[ "$useHeadNode" == "nhn" ]]; then

    #mpirun -n 8 -ppn 1  ./dw2_testOSPRay_impi /home/sci/feng/Desktop/ws/data/COSMOS_ospray/cosmos.osp \
    mpirun -n 8 -ppn 1 -hosts sdvis1,sdvis2,sdvis3,sdvis4,sdvis5,sdvis6,sdvis7,sdvis8 \
        ./dw2_testOSPRay_impi /home/sci/feng/Desktop/ws/data/COSMOS_ospray/cosmos.osp \
            -iso 0.0 \
            -vp 814.398254 -54.506733 878.914001 \
            -vu 1.000000 0.000000 0.000000 \
            -vi 248.845215 267.241455 256.334686 \
	    -fif 10\
            -hostname powerwall01 -port 2903 --osp:mpi
            -hostname 155.98.19.223 -port 2903 --osp:mpi
else

    mpirun  -n 8 -ppn 1 -hosts sdvis1,sdvis2,sdvis3,sdvis4,sdvis5,sdvis6,sdvis7,sdvis8 \
        ./dw2_testOSPRay_impi /home/sci/feng/Desktop/ws/data/COSMOS_ospray/cosmos.osp \
            -iso 0.0 \
            -vp 814.398254 -54.506733 878.914001 \
            -vu 1.000000 0.000000 0.000000 \
            -vi 248.845215 267.241455 256.334686 \
            -hostname han.sci.utah.edu -port 2903 --osp:mpi
fi
