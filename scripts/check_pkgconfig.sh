#!/usr/bin/env sh

if [ ! $# -eq 1 ]; then
  1>&2 echo "Usage: $0 <pkg config>"
  exit 1
fi

pkg-config "$1" && echo y || echo n

