#!/bin/bash

docker run -v $(pwd):/app -w /app smatyukevich/raspberry-pi-os-builder make $1
