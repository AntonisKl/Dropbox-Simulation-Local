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
dataBytesWritten=0
dataBytesRead=0
metadataBytesWritten=0
metadataBytesRead=0
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
        ((dataBytesWritten += ${line[11]}))
        ((filesWritten ++))
    elif [[ ${line[4]} == "received" ]] ; then
        # echo $pidsIndex
        ((dataBytesRead += ${line[11]}))
        ((filesRead ++))
    elif [[ ${line[4]} == "read" ]] ; then
        ((metadataBytesRead += ${line[5]}))
    elif [[ ${line[4]} == "wrote" ]] ; then
        ((metadataBytesWritten += ${line[5]}))
    fi

    if [[ ${line[0]} == "Client" ]] ; then
        if [[ ${line[7]} == "exited" ]] ; then
           ((clientsExited ++))
        elif [[ ${line[7]} == "logged" ]] && ! arrayContains "${line[6]}" "${pids[@]}" ; then
            # echo $pidsIndex
            pids[pidsIndex]=${line[6]}
            # echo Added pid ${line[3]} to pid array
            ((pidsIndex ++))
        fi
    fi
done

# print stats
echo
echo Client pids:
printf '\t%s\n' "${pids[@]}"

IFS=$'\n'
pidsSorted=($(sort -n <<< "${pids[*]}"))
unset IFS

echo
echo Total number of client pids: ${#pidsSorted[@]}
echo Max client pid: ${pidsSorted[${#pidsSorted[@]}-1]}
echo Min client pid: ${pidsSorted[0]}
echo
echo Metadata bytes written: $metadataBytesWritten, Metadata bytes read: $metadataBytesRead
echo Data bytes written: $dataBytesWritten, Data bytes read: $dataBytesRead
echo Files written: $filesWritten, Files read: $filesRead
echo Clients exited: $clientsExited