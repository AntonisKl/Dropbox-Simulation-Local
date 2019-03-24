#!/bin/bash

if [[ "$#" -ne 4 ]]; then
    echo "Illegal number of parameters";  exit 1
fi

dirName = $0
filesNum = $1
dirsNum = $2
levelsNum = $3

args=("$@")

integerRegex='^[0-9]+$'

for arg in "${args[@]}" do
  if ! [[ $arg == $0 ]]
    continue
  fi

  if ! [[ $arg =~ $integerRegex ]]; then
     echo "Argument $arg is not an integer"; exit 1
  fi
done

# create directory if it does not exist
mkdir -p $dirName

dirNames = ()
dirFullNames = ()
fileNames = ()

for ((i=1; i<=$filesNum; i++)); do
  fileNames[i] = head /dev/urandom | tr -dc A-Za-z0-9 | head -c (shuf -i 1-8 -n 1) ; echo ''
done

levelsNumTemp = 0
for ((i=1; i<=$dirsNum; i++)); do
   dirNames[i] = head /dev/urandom | tr -dc A-Za-z0-9 | head -c (shuf -i 1-8 -n 1) ; echo ''

   curDirName = ${dirNames[i]}
   for ((j=0; j<=$levelsNumTemp; j++)); do
     curDirName = "${dirNames[i - j]}/$curDirName"
   done

   mkdir -p $curDirName

   dirFullNames[i] = $curDirName

   ((levelsNumTemp ++))

   if [[ $levelsNumTemp == $levelsNum ]]
    levelsNumTemp = 0
   fi
done
