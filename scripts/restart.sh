#!/bin/bash
echo "killing process"
kill -9 $(cat server.pid)
rm server
cc server.c -lrt -o server 
./server 2002 ./public/
