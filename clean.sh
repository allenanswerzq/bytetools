#!/usr/bin/env bash
for dir in $(ls -d */ | cut -f1 -d'/' | grep -v third | grep -v bits); do
    find $dir -type f -perm +0111 | xargs -I{} -t rm -rf {}
done
