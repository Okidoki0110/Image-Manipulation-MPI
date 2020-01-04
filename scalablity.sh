#!/bin/bash
if [ "$#" -ne 4 ]; then
	echo "Illegal numer of arguments"
	echo "The first argument should be the name of the executable"
	echo "The second argument should be the image you want to scale"
	echo "The third argument should be the reference output image"
	echo "The forth argument should be the number of times to run the program"
	exit
fi

bssembssem="blur smooth sharpen emboss mean blur smooth sharpen emboss mean
"
numberProcesses=8
AVG1=0
AVG4=0
quit=0

if [[ $2 == *.pgm ]]; then
	output="output.pgm"
else
	output="output.pnm"
fi

echo "Checking only with bssembssem."

for i in $(seq 1 1 $4)
do
	TIMEFORMAT=%R
	TIME1=$( time ( eval mpirun -np 1 $1 $2 $output "$bssembssem" &> /dev/null ) 2>&1 );
	x=$(echo $TIME1 | tr , '.');
	
	echo -n "$x "
	AVG1=$(echo "$AVG1 + $x" | bc -l | cut -c1-5)
	rm $output
done

if [ $quit == 0 ]; then
	echo ""
	AVG1=$(echo "$AVG1 / $4" | bc -l | cut -c1-5)
	echo "Average for 1 process: $AVG1"

	for i in $(seq 1 1 $4)
	do
		TIMEFORMAT=%R
		TIME4=$( time ( eval mpirun -np 8 $1 $2 $output "$bssembssem" &> /dev/null ) 2>&1 );
		x=$(echo $TIME4 | tr , '.');
		
		echo -n "$x "
		AVG4=$(echo "$AVG4 + $x" | bc -l | cut -c1-5)
		rm $output
	done

	if [ $quit == 0 ]; then
		echo ""
		AVG4=$(echo "$AVG4 / $4" | bc -l | cut -c1-5)
		echo "Average for 4 processes: $AVG4"
		SCAL=$(echo "($AVG1 - $AVG4) / $AVG1" | bc -l | cut -c1-5)
		echo $SCAL
	fi
fi

