#!/bin/bash
size=$(stat -c%s $1)
curl -X POST http://localhost:2003 -H 'Content-Type: multipart/form-data' -H 'Content-Length: '$size -H 'Content-Disposition: form-data; name="file"; filename="'$1'"'  -T $1 -o /dev/stdout
