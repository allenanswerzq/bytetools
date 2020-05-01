#!/usr/bin/env bash
set -e
for dir in $(ls -d */ | cut -f1 -d'/' | grep -v third | grep -v bits); do
    find $dir -type f | perl -lne 'print if -B' | xargs -I{} -t rm -rf {}
done
