#!/bin/bash
for i in $(seq 1 10000); do
	curl -o /dev/null http://localhost:8000/index.html & 
	curl -o /dev/null http://localhost:8000/img/404.jpg &
	./httpreq.sh test.txt &
done
