#!/bin/bash

if [[ "$#" -ne 3 ]]; then
    echo "Illegal number of parameters";  exit 1
fi

clientsNum=$1
delay=$2
bufferSize=$3

pkill -f mirror_client

rm -rf mirror1 mirror2 mirror3 mirror4 mirror5 mirror6 mirror7 mirror8 mirror9 mirror10 log*

make clean && make

sleep 2

for ((i=1; i<=$clientsNum; i++));
do
    inputDirNum=$((($i - 1)%2 + 1))
    gnome-terminal --window-with-profile=test --working-directory=/home/antonis/Documents/syspro2/ -e "./exe/mirror_client -n $i -c common -i input$inputDirNum -m mirror$i -b $bufferSize -l log$i.txt"
    sleep $delay
done

sleep $(($clientsNum*3))

cat log* | ./bash_scripts/get_stats.sh