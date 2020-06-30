#!bin/bash

useHeadNode="$1"
echo "$useHeadNode"

if [[ "$useHeadNode" == "nhn" ]]; then

	mpirun -ppn 1 -n 36 \
		-hosts powerwall01,powerwall02,powerwall03,powerwall04,powerwall05,powerwall06,powerwall07,powerwall08,powerwall09,powerwall01,powerwall02,powerwall03,powerwall04,powerwall05,powerwall06,powerwall07,powerwall08,powerwall09,powerwall01,powerwall02,powerwall03,powerwall04,powerwall05,powerwall06,powerwall07,powerwall08,powerwall09,powerwall01,powerwall02,powerwall03,powerwall04,powerwall05,powerwall06,powerwall07,powerwall08,powerwall09 \
		-genv DISPLAY=:0 ./dw2_service -nhn \
		--num-displays 9 4 \
		--window-size 2560 1440 \
		-fif 10\
		--displays-per-node 4 0 0 4320 0 0 2880 0 0 1440 0 0 0
else
	mpirun -ppn 1 -n 37 \
		-hosts powerwall00,powerwall01,powerwall02,powerwall03,powerwall04,powerwall05,powerwall06,powerwall07,powerwall08,powerwall09,powerwall01,powerwall02,powerwall03,powerwall04,powerwall05,powerwall06,powerwall07,powerwall08,powerwall09,powerwall01,powerwall02,powerwall03,powerwall04,powerwall05,powerwall06,powerwall07,powerwall08,powerwall09,powerwall01,powerwall02,powerwall03,powerwall04,powerwall05,powerwall06,powerwall07,powerwall08,powerwall09 \
		-genv DISPLAY=:0 ./dw2_service -hn \
		--num-displays 9 4 \
		--window-size 2560 1440 \
		-fif 10\
		--displays-per-node 4 0 0 4320 0 0 2880 0 0 1440 0 0 0
fi

		#-bw 81.7 83.07\
