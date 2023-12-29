#!/usr/bin/env sh

if [ ! $# -eq 1 ]; then
  1>&2 echo "Usage: $0 <lib name>"
  exit 1
fi

TEMP=$(mktemp)
trap "rm -- $TEMP" EXIT

MUTED=$(echo "int main() {}" | $CC -Wl,--no-as-needed -l$1 -xc -o $TEMP - 2>&1)
(exit $?) && echo y || echo n

