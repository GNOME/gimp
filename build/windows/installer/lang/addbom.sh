#!/bin/bash

sed "1s/^/\xEF\xBB\xBF/" "$1" > "$2"
