#!/bin/bash
set -e
cd "$(dirname "$0")/project/emscripten"
python3 build_web.py
