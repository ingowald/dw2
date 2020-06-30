#!bin/bash

useHeadNode="$1"
echo "$useHeadNode"

if [[ "$useHeadNode" == "nhn" ]]; then

    mpirun -n 8 -ppn 1 --hosts sdvis1,sdvis2,sdvis3,sdvis4,sdvis5,sdvis6,sdvis7,sdvis8 \
        ./dw2_testFrameRenderer 155.98.19.61 2903 --tile-size 256 

else
    mpirun -n 8 -ppn 1 --hosts sdvis1,sdvis2,sdvis3,sdvis4,sdvis5,sdvis6,sdvis7,sdvis8 \
        ./dw2_testFrameRenderer 155.98.19.60 2903 --tile-size 256 
fi
