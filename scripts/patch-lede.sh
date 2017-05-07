#!/bin/bash

for q in ../lede-linino-patches/*.patch
do
        git am $q --ignore-whitespace --no-scissors --ignore-space-change
done
