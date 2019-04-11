#!/bin/bash
# Clean up the GPG Keyring.  Keep it tidy.
# blog.lavall.ee

echo -n "Expired Keys: "
for expiredKey in $(gpg --list-keys); do
    re='^[0-9]+$'
    # if ! [[ $expiredKey =~ $re ]] ; then
    #     continue
    # fi
    STRLENGTH=$(echo -n $expiredKey | wc -m)

    # if [[ STRLENGTH < 15 ]]; then
    #     continue
    # fi

    echo "$expiredKey"
    gpg --yes --batch --quiet --delete-secret-keys $expiredKey
    gpg --yes --batch --quiet --delete-keys $expiredKey
    if [ $? -eq 0 ]; then
        echo -n "(OK), "
    else
        echo -n "(FAIL), "
    fi
done
echo done.