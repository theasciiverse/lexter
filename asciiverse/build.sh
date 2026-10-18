#!/bin/sh
# Builds Lexter into out/: the seven sources in one compiler run (one
# process, so no parallel jobs to limit), with asciiverse/config.h instead
# of the 2001 configure script. The compiler, flags and curses library come
# from the environment, so one script serves every target (see
# .github/workflows/release.yml):
#
#   asciiverse/build.sh                                       # Linux
#   LDFLAGS="-static -s" asciiverse/build.sh                  # Alpine, static
#   CURSES_LIBS=-lncurses CFLAGS="-O2 -arch arm64" \
#     LDFLAGS="-arch arm64" asciiverse/build.sh               # macOS
#
# The dictionaries are copied beside the binary (out/dict.en, out/dict.fr).
set -eu
cd "$(dirname "$0")/.."
CC="${CC:-cc}"
CFLAGS="${CFLAGS:--O2}"
LDFLAGS="${LDFLAGS:-}"
CURSES_LIBS="${CURSES_LIBS:--lncursesw}"
mkdir -p out
rm -f out/lexter
# shellcheck disable=SC2086
nice -n 10 $CC -std=gnu99 -Wall $CFLAGS -Iasciiverse -Isrc -o out/lexter \
  src/lexter.c src/words.c src/dict.c src/text.c src/list.c src/scores.c src/lang.c src/paths.c \
  $CURSES_LIBS $LDFLAGS
cp data/dict.en data/dict.fr out/
sed -e 's|@datadir@/lexter|/usr/share/lexter|g' \
  -e 's|@VERSION@|1.0.3|g' src/lexter.6.in > out/lexter.6
echo "built lexter"
