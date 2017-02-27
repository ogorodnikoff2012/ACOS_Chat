#!/bin/bash
for i in `cat -`; do ./sleep_and_echo.sh $i & done
sleep .04
while [ -f ".threads" ]; do
    sleep .01
done
