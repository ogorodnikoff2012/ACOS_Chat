#!/bin/bash
for i in `cat -`; do ./sleep_and_echo.sh $i & done
