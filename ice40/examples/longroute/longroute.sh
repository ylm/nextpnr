#!/usr/bin/env bash
set -ex
yosys -p "synth_ice40 -top top -json longroute.json" longroute.v
../../../nextpnr-ice40 --up5k --json longroute.json --pcf longroute.pcf --asc longroute.asc --pre-route router.py
icepack longroute.asc longroute.bin
iceprog longroute.bin
