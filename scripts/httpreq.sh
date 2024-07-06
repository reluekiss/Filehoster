#!/bin/bash
size=$(stat -c%s $2)
curl -X POST $1 -H 'Content-Length: '$size -T $2 --output output.webm
