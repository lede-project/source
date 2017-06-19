#!/bin/bash

for q in `ls package/linino | grep node-`
do
    make package/$q/compile V=99
done
