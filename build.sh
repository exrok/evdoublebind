#!/bin/sh
musl-gcc -static -flto -O3 ./evdoublebind.c -o evdoublebind
