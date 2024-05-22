#!/bin/bash
curl -X POST localhost:2002 -f file test.txt
curl -X GET localhost:2002
