#!/bin/sh
# Packs the build into dist/lexter-<target>.tar.gz for av: the binary at
# the top with the dictionaries beside it (where it looks first), the man
# page, README, COPYING and PATCHES.md.
#
#   asciiverse/package.sh x86_64-unknown-linux-musl
set -eu
cd "$(dirname "$0")/.."
target="$1"
stage=stage/lexter
rm -rf "$stage" && mkdir -p "$stage" dist
cp out/lexter out/dict.en out/dict.fr out/lexter.6 README COPYING PATCHES.md "$stage/"
tar -czf "dist/lexter-$target.tar.gz" -C "$stage" .
ls dist
