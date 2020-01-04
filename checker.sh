#!/bin/bash
if [ "$#" -ne 3 ]; then
	echo "Illegal numer of arguments"
	echo "The first argument should be the name of the executable"
	echo "The second argument should be the directory where the PNM,PGM directories with input images
		are"
	echo "The third argument should be the directory where the pnm/pgm directories with reference 
		output images are"
	exit
fi
filters=(
	'smooth'
	'blur'
	'sharpen'
	'mean'
	'emboss'
	"blur smooth sharpen emboss mean blur smooth sharpen emboss mean"
	)
echo "Order of filters is: smooth, blur, sharpen, mean, emboss, bssembssem."
for d in $2/*; do
	for file in "$d"/*; do
		type=$(echo $file| cut -d '/' -f2 | tr '[:upper:]' '[:lower:]')
		name=$(basename $file)
		extension=$(echo "$name"| cut -d '.' -f2)
		if [[ "$extension" == "$type" ]]; then
			echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
			echo "$name"
			noext=$(echo "$name" | cut -d "." -f1)
			for filter in "${filters[@]}"; do
				output="$noext-$filter.$extension"
				if [[ "$filter" == "blur smooth sharpen emboss mean blur smooth sharpen emboss mean" ]]; then
					output="$noext-bssembssem.$extension"

					TIMEFORMAT=%R
					TIME1=$(time ( eval mpirun -np 1 $1 "$file" "1proc" "$filter" &> /dev/null ) 2>&1 );

					TIMEFORMAT=%R
					TIME4=$(time ( eval mpirun -np 4 $1 "$file" "$output" "$filter" &> /dev/null ) 2>&1 );

					x=$(echo $TIME1 | tr , '.');
					y=$(echo $TIME4 | tr , '.');
					TIME=$(echo "($x - $y) / $x" | bc -l | cut -c1-5)
				else
					eval mpirun -np 1 $1 "$file" "1proc" "$filter" &> /dev/null
					eval mpirun -np 4 $1 "$file" "$output" "$filter" &> /dev/null
				fi
				eval mpirun -np 2 $1 "$file" "2proc" "$filter" &> /dev/null
				eval mpirun -np 3 $1 "$file" "3proc" "$filter" &> /dev/null
				res=$(diff "$output" "$3"/"$extension"/"$output" && diff "1proc" "2proc" && diff "2proc" "3proc" && diff "3proc" "$output");
				if [ "$res" == "" ]; then
					echo "PASSED.................................1/1"
					if [[ "$filter" == "blur smooth sharpen emboss mean blur smooth sharpen emboss mean" ]]; then
						echo "$TIME1 $TIME4 $TIME"
					fi
					SUM=$((SUM + 1));
				else
					echo "FAILED.................................0/1"
				fi
				rm "$output" "1proc" "2proc" "3proc"
			done
		fi
	done
done
echo "TOTAL................................$SUM/60"
