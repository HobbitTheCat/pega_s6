#!/bin/bash
sum=0

for i in $(find . -type f -name "*.[c,h]");do
    lign=$(wc -l < "$i")
    sum=$((sum+lign))
  done
echo $sum








