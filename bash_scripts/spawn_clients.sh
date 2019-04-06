#!/bin/bash

if [[ "$#" -ne 2 ]]; then
    echo "Illegal number of parameters";  exit 1
fi

clientsNum=$1
delay=$2

rm -rf mirror1 mirror2 mirror3 mirror4

make clean && make

sleep 2

for ((i=1; i<=$clientsNum; i++));
do
    inputDirNum=$(($i%2 + 1))
    gnome-terminal --window-with-profile=test --working-directory=/home/antonis/Documents/syspro2/ -e "./exe/mirror_client -n $i -c common -i input$inputDirNum -m mirror$i -b 1000 -l log$i.txt"
    sleep $delay
done