#!/bin/bash

socat -d -d TCP-LISTEN:3333,reuseaddr,fork FILE:/dev/cu.usbserial-0001,raw,echo=0
