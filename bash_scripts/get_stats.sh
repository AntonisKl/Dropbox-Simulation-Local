#!/bin/bash

ids=() # array of the client ids that will be found in the log files
idsIndex=0 # next index of the ids array in each while loop
dataBytesWritten=0 # total number of bytes of file contents written
dataBytesRead=0 # total number of bytes of file contents read
metadataBytesWritten=0 # total number of bytes of metadata written (e.g. file path's size, ending symbol)
metadataBytesRead=0 # total number of bytes of metadata read (e.g. file path's size, ending symbol)
filesWritten=0 # total number of files written
filesRead=0 # total number of files read
clientsExited=0 # clients that exited successfully

while read -a line ;
do
    if [[ ${line[4]} == "sent" ]] ; then    
        ((dataBytesWritten += ${line[11]})) # add to written data
        ((filesWritten ++))                 # add to written files
    elif [[ ${line[4]} == "received" ]] ; then
        ((dataBytesRead += ${line[11]}))    # add to read data
        ((filesRead ++))                    # add to read files
    elif [[ ${line[4]} == "read" ]] ; then
        ((metadataBytesRead += ${line[5]}))    # add to read metadata
    elif [[ ${line[4]} == "wrote" ]] ; then
        ((metadataBytesWritten += ${line[5]})) # add to written metadata
    fi

    if [[ ${line[0]} == "Client" ]] ; then
        if [[ ${line[7]} == "exited" ]] ; then
           ((clientsExited ++))                 # add to exited clients
        elif [[ ${line[7]} == "logged" ]] ; then
            ids[idsIndex]=${line[6]}            # add to current client's id to ids array
            ((idsIndex ++))                     # proceed index of ids array
        fi
    fi
done

# sort ids array and store it to idsSorted array
IFS=$'\n'
idsSorted=($(sort -n <<< "${ids[*]}"))
unset IFS

# print stats
echo    # newline
echo Client ids:
printf '\t%s\n' "${idsSorted[@]}"

echo    # newline
echo Total number of client ids: ${#idsSorted[@]}
echo Max client id: ${idsSorted[${#idsSorted[@]}-1]}
echo Min client id: ${idsSorted[0]}
echo    # newline
echo Metadata bytes written: $metadataBytesWritten, Metadata bytes read: $metadataBytesRead
echo Data bytes written: $dataBytesWritten, Data bytes read: $dataBytesRead
echo Total bytes written: $(($metadataBytesWritten + $dataBytesWritten)), Total bytes read: $(($metadataBytesRead + $dataBytesRead))
echo Files written: $filesWritten, Files read: $filesRead
echo Clients entered: ${#idsSorted[@]}
echo Clients exited: $clientsExited