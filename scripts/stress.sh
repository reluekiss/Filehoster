#!/bin/bash
for i in $(seq 1 5000); do
	curl -o /dev/null http://localhost:2002/index.html & 
	curl -o /dev/null http://localhost:2002/404.jpg &
	./httpreq.sh test.txt &
done
