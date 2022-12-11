#!/bin/bash

DELIMITER=";"

VERSION=$1

OUTPUT=./results/$VERSION.csv

rm $OUTPUT &>/dev/null

for nodes in 1 2 4 8 # 16 32
do
	for (( i=0; i < 10; i++ ))
	do
		./mpi.sh $nodes $VERSION > temp
		awk '/It took/{printf "%s", $(NF-1)}' temp >> $OUTPUT
		rm temp
		echo -n $DELIMITER >> $OUTPUT
	done
done

sed -i '$ s/.$//' $OUTPUT
