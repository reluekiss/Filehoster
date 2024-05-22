#!/bin/bash
echo "killing process"
kill -9 $(cat ./server.pid)
rm server
make
./server 2002 ./public/
