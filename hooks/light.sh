#!/usr/bin/env bash
# Usage: light.sh w|n|r|i   (working / needs you / ready / idle)
# Port: set CLAUDE_LIGHT_PORT, otherwise auto-detects the Flora.
cmd="$1"
case "$cmd" in w|n|r|i) ;; *) exit 0 ;; esac

port="${CLAUDE_LIGHT_PORT:-}"
if [ -z "$port" ]; then
  for p in /dev/serial/by-id/*Flora* /dev/serial/by-id/*Adafruit* /dev/ttyACM0; do
    [ -e "$p" ] && { port="$p"; break; }
  done
fi
[ -n "$port" ] && [ -w "$port" ] || exit 0   # never break Claude if the light is unplugged

stty -F "$port" 9600 raw -echo 2>/dev/null
printf '%s' "$cmd" > "$port" 2>/dev/null
exit 0
