#!/bin/bash

arrayContains() {
    element=$1
    shift
    array=("$@")
    for i in "${array[@]}"
    do
        # echo pids $element $i
        if [ "$i" == "$element" ] ; then
            # echo 1
            return 0
        fi
    done
    # echo 0
    return 1
}

pids=()
pidsIndex=0
bytesWritten=0
bytesRead=0
filesWritten=0
filesRead=0
clientsExited=0

while read -a line ;
do
    # echo "${line[3]}";
    # echo $(arrayContains "${pids[@]}" "${line[3]}")
    
    # echo ${line[4]}
    if [[ ${line[4]} == "sent" ]] ; then
        # echo $pidsIndex
        ((bytesWritten += ${line[11]}))
        ((filesWritten ++))
    elif [[ ${line[4]} == "received" ]] ; then
        # echo $pidsIndex
        ((bytesRead += ${line[11]}))
        ((filesRead ++))
    fi

    if [[ ${line[0]} == "Client" ]] ; then
        if [[ ${line[4]} == "exited" ]] ; then
           ((clientsExited ++))
        elif [[ ${line[4]} == "logged" ]] && ! arrayContains "${line[3]}" "${pids[@]}" ; then
            # echo $pidsIndex
            pids[pidsIndex]=${line[3]}
            # echo Added pid ${line[3]} to pid array
            ((pidsIndex ++))
        fi
    fi
done

# print stats
echo
echo Pids:
printf '\t%s\n' "${pids[@]}"

IFS=$'\n'
pidsSorted=($(sort <<< "${pids[*]}"))
unset IFS

echo
echo Total number of pids: ${#pidsSorted[@]}
echo Max pid: ${pidsSorted[${#pidsSorted[@]}-1]}
echo Min pid: ${pidsSorted[0]}
echo
echo Bytes written: $bytesWritten, Bytes read: $bytesRead
echo Files written: $filesWritten, Files read: $filesRead
echo Clients exited: $clientsExited