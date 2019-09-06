#!/bin/bash

echo "Boot up script"

if ! [ $(id -u) = 0 ]; then
   echo "The script need to be run as root." >&2
   exit 1
fi

mkdir -p grafana/data
 
chown 472:472 grafana/data

docker-compose up -d

echo "Start Complete"