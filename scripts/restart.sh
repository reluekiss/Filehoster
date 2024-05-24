#!/bin/bash
echo "killing process"
kill -9 $(cat server.pid)
rm server
cc -o server server.c randString.c
./server 2002 public
