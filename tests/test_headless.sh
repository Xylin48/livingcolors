#!/bin/sh
set -eu
dir=$(mktemp -d)
trap 'rm -rf "$dir"' EXIT HUP INT TERM
bin=./build/livingcolors-headless
{
    printf 'capture\nsave\nwalk 1 0 0.4\ntick 100\ncapture\nsense 1 0\nload\ncapture\n'
    printf 'walk NaN 0 1\ntick 4294967296\nuse 110 999\n'
    printf '%0300dquit\n' 0
    printf 'sense 5 0\nquit\n'
} | "$bin" "$dir/state" "$dir/frame.ppm" "$dir/audio.wav" > "$dir/log"
grep -q '^READY generation=1 revision=0 ms=0$' "$dir/log"
grep -q '^CAPTURED generation=1 revision=2 ms=100$' "$dir/log"
grep -q '^LOADED generation=2 revision=3 ms=0$' "$dir/log"
grep -q '^CAPTURED generation=2 revision=3 ms=0$' "$dir/log"
grep -q '^ENV zone=' "$dir/log"
grep -q '^AUDIO sources=' "$dir/log"
test "$(grep -c '^INVALID$' "$dir/log")" -eq 4
test "$(wc -c < "$dir/frame.ppm" | tr -d ' ')" -eq 921615
test "$(wc -c < "$dir/audio.wav" | tr -d ' ')" -eq 6444
printf 'P6\n640 480\n255\n' > "$dir/header"
head -c 15 "$dir/frame.ppm" | cmp - "$dir/header"
printf 'RIFF' > "$dir/header"
head -c 4 "$dir/audio.wav" | cmp - "$dir/header"
printf 'capture\nquit\n' | "$bin" "$dir/unused" "$dir/fresh.ppm" "$dir/fresh.wav" > /dev/null
cmp "$dir/frame.ppm" "$dir/fresh.ppm"
cmp "$dir/audio.wav" "$dir/fresh.wav"
echo 'Living Colors: standalone console, RGB/PCM file capture and restore smoke tests passed'
