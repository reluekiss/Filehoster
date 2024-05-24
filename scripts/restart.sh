#!/bin/bash
echo "killing process"
kill -9 $(cat server.pid)
rm server
cc -o server -lrt server.c 
./server 2003 public
