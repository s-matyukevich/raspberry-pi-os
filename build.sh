#!/bin/bash

if [[ -z "$1" ]]; then
  echo "Ussage: ./build.sh <lessonXX>"
  echo "For example: ./build.sh lesson01"
  exit 1
fi

docker run -it -v $(pwd)"/src/$1:/app" smatyukevich/raspberry-pi-os-builder "cd app && make"
