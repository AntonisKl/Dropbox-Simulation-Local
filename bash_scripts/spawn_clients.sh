#!/bin/bash

if [[ "$#" -ne 1 ]]; then
    echo "Illegal number of parameters";  exit 1
fi

clientsNum=$1

rm -rf mirror1 mirror2

make clean && make

sleep 2

for ((i=1; i<=$clientsNum; i++));
do
    inputDirNum=$(($i%2 + 1))
    gnome-terminal --working-directory=/home/antonis/Documents/syspro2 -e "./exe/mirror_client -n $i -c common -i inputDir$inputDirNum -m mirror$i -b 10 -l logFile"
    sleep 3
done