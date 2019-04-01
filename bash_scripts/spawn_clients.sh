#!/bin/bash

if [[ "$#" -ne 1 ]]; then
    echo "Illegal number of parameters";  exit 1
fi

clientsNum=$1

rm -rf mirror1 mirror2

# while true
# do
    make clean && make

    sleep 2

    for ((i=1; i<=$clientsNum; i++));
    do
        ./exe/mirror_client -n $i -c common -i inputDir$(($i%2 + 1)) -m mirror$i -b 10 -l logFile &
        sleep 2
        # echo "${args[$i]}"
    done

#     sleep 4
# done

