#!/bin/bash
array=(W W W W W W F S)
row=$1;
col=$2;
file=planet.dat
echo $row > $file;
echo $col >> $file;

for (( rowindex = 0; rowindex < $row; rowindex++ )); do
	for (( colindex = 0; colindex < $col; colindex++ )); do
		echo -n ${array[$RANDOM %8]} >> $file ;
		if (("$colindex" < "$col"-1));
		then
			echo -n ' ' >> $file;
		else
			echo '' >> $file;
		fi
	done
done
