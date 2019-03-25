#!/bin/bash

if [[ "$#" -ne 4 ]]; then
    echo "Illegal number of parameters";  exit 1
fi

dirName=$1
filesNum=$2
dirsNum=$3
levelsNum=$4

args=("$@")

integerRegex='^[0-9]+$'

for arg in "${args[@]}"
do
    if [[ $arg == $1 ]] || [[ $arg == $0 ]]; then
        continue
    fi

    if ! [[ $arg =~ $integerRegex ]]; then
        echo "Argument $arg is not an integer"; exit 1
    fi
done

# create directory if it does not exist
mkdir -p $dirName

dirNames=()
dirFullNames=()
fileNames=()

for ((i=1; i<=$filesNum; i++));
do
    charsNum=$(shuf -i 1-8 -n 1)
    fileNames[i]=$(head /dev/urandom | tr -dc A-Za-z0-9 | head -c $charsNum ; echo '')
    # echo "${args[$i]}"
done

filesNumPerDir=$(( $filesNum / $dirsNum ))
if (( $filesNumPerDir == 0 )); then
    filesNumPerDir=1
fi

echo "filesNumPerDir=$filesNumPerDir"

levelsNumTemp=0
fileNamesIndex=1
for ((i=1; i<=$dirsNum; i++));
do
    charsNum=$(shuf -i 1-8 -n 1)
    dirNames[i]=$(head /dev/urandom | tr -dc A-Za-z0-9 | head -c $charsNum ; echo '')

    curDirName=${dirNames[i]}
    for ((j=1; j<=$levelsNumTemp; j++));
    do
        curDirName="${dirNames[i - j]}/$curDirName"
    done

    curDirName="$dirName/$curDirName"
    echo "$curDirName"

    mkdir -p $curDirName

    for ((j=0; j<$filesNumPerDir; j++));
    do
        if (( $fileNamesIndex > $filesNum )); then
            break
        fi

        touch "$curDirName/${fileNames[$fileNamesIndex]}"

        ((fileNamesIndex ++))
    done

    dirFullNames[i]=$curDirName

    ((levelsNumTemp ++))

    if (( $levelsNumTemp == $levelsNum )); then
        levelsNumTemp=0
    fi
done

echo "filesIndex = $fileNamesIndex"

# create remaining files
while (( $fileNamesIndex <= $filesNum ));
do
    for dirFullName in "${dirFullNames[@]}"
    do
        for ((j=0; j<$filesNumPerDir; j++));
        do
            if (( $fileNamesIndex > $filesNum )); then
                break
            fi

            touch "$dirFullName/${fileNames[$fileNamesIndex]}"

            ((fileNamesIndex ++))
        done

        if (( $fileNamesIndex > $filesNum )); then
            break
        fi
    done

    if ((  $fileNamesIndex > $filesNum )); then
        break
    fi
done