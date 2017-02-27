#!/bin/bash

lock() {
    while ! mkdir ".lock" 2>/dev/null; do
        sleep .01
    done
}

unlock() {
    rmdir ".lock"
}

inc_thread_cnt() {
    lock
    if ! [ -f ".threads" ]; then
        echo 1 > ".threads"
        export thread_num=0
    else
        export thread_num=`cat ".threads"`
    fi
    echo "$thread_num + 1" | bc > ".threads"
    unlock
}

dec_thread_cnt() {
    lock
    export thread_num=`cat ".threads"`
    echo "$thread_num - 1" | bc > ".threads"
    if [[ $thread_num == 1 ]]; then
        rm ".threads"
    fi
    unlock
}

inc_thread_cnt
echo -e "scale = 5\n($1 + 1) / 25" | bc | xargs sleep
echo -n "$1 "
dec_thread_cnt
if ! [ -f ".threads" ]; then
    echo
fi
