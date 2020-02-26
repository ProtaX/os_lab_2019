#!/bin/sh

#WARNING
#before you run this script you have to cd to /lab1 dir

#Task 1
./background.sh &
#Task 2
grep "cake" cake_rhymes.txt | cat > with_cake.txt
#Task 3
chmod +x hello.sh && ./hello.sh
pwd && date && echo $PATH
#Task 4
chmod +x average.sh && ./average.sh 1 2 3 4
gcc main.c -lm -o rng.out
./rng.out 150 numbers.txt && ./average.sh $(cat numbers.txt)
