# Changes from upstream

This is the Asciiverse packaging of Lexter 1.0.3, Mark Pulford's
real-time word puzzle for text terminals (C and curses, 1998 to 2001):
arrange falling letters into words. 1.0.3 (2001-03-23) is the last
release. His page is still up at https://www.kyne.au/~mark/software/lexter.php
(formerly kyne.com.au), with the same tarball.

History here starts from the release tarball (lexter-1.0.3.tar.gz, sha256
b61a28fd5249b7d6c0df9be91c97c2acd00ccd9ad1e7b0c99808f6cdc96d5188, the same
file on Mark Pulford's site, on ibiblio under pub/Linux/games/arcade, and
the one FreeBSD and Termux pin), unchanged, tagged `1.0.3`.

## Licence

The source files say "This file is subject to the terms and conditions of
the GNU General Public License. Read the file COPYING found in this
archive", without a version; README says "Lexter is covered by the GNU
GPL"; COPYING is the GPL version 2. The catalog says GPL-2.0-only, the
version shipped.

## The dictionaries

`data/dict.en` (31,559 words, ASCII) and `data/dict.fr` (50,262 words,
ISO 8859-1), all 4 to 10 letters long. Neither the archive (README,
ChangeLog, NEWS, the spec file) nor Mark Pulford's page says where the
word lists come from; they are covered by the archive's GPL like the rest.

## Distribution patches

Repology (2026-10-18) lists two packages, both 1.0.3: FreeBSD's
games/lexter (no patch files) and Termux's lexter. Termux's two patches
are here as one commit each with Henrik Grimler's name (both added
2020-05-10, termux-packages `63bae47`), tagged `termux/1.0.3-5`:

- `configure.in.patch`: getwd is no longer required (Bionic and musl do
  not have it).
- `acinclude.m4.patch`: AM_LC_MESSAGES carried in acinclude.m4, which newer
  gettext no longer provides, so autoreconf works.

Termux regenerates configure; the Asciiverse build does not use it (see
Packaging), so these change nothing in our binaries.

## Ours

- **Scores and screenshots in the player's folder; dictionaries found at
  run time.** The high score table was one shared file in LOCALSTATEDIR
  (a setgid program or a world-writable file), and screenshots went to the
  current folder (av runs games in their install folder). Both now go in
  `$XDG_DATA_HOME/lexter` (else `~/.local/share/lexter`). The dictionaries
  were looked for only in the DATADIR fixed at build time; now
  `$LEXTER_DICTDIR`, else the program's own folder when dict.en is there
  (the av archive), else `/usr/share/lexter`. New file src/paths.c; the
  man page updated.
- **Language from the two-letter code, English quietly as the fallback.**
  `LANG=C.UTF-8` was read as the language "C." and every start showed
  "ERROR: Unable to load C. dictionary, using en instead", as did every
  language without a dictionary. Only a two-letter code counts now, and a
  missing dictionary for the player's language falls back to English
  without the error; `-d` still reports one.
- **The high score name from the environment.** It was always the
  account name from getpwuid; now `$USER`, else `$LOGNAME`, else the
  account name, so a player's own name shows and a recording made with
  `USER=player` does not show the recording machine's login.
- **Centred in the terminal, also after a resize; keys through curses.**
  The 80x24 screen is drawn on a window centred in the terminal (the
  stdscr calls in text.c pointed at it), centred again on KEY_RESIZE and
  refilled from a pad copy if a smaller terminal cut it down. Keys were
  waited for with select() on a second /dev/tty descriptor, so keys curses
  had already buffered could wait a tick and a resize never woke it; the
  wait is now curses' own timeout.
- **French on UTF-8 terminals.** 18,805 of the 50,262 French words have
  Latin-1 accents (é, è, ê, à, ...), which a UTF-8 terminal showed as
  broken characters. The locale is always set now, and on a UTF-8
  terminal every byte the game draws goes out as the Unicode character
  with that number (Latin-1 is the first 256); keys are read as wide
  characters, so an é typed for a blank arrives as one key. Other
  terminals get bytes as before; the dictionaries are unchanged.

Windows: not built. It would need LockFileEx instead of flock for the
score file, GetModuleFileName instead of /proc/self/exe, and a wide
PDCurses.

## Packaging (new files)

- `asciiverse/config.h`, `asciiverse/build.sh`: one compiler call with a
  fixed config.h instead of the 2001 configure script (which looks for
  getwd and an old gettext), no NLS (Lexter has no translations, po/ holds
  only the template); the dictionaries copied beside the binary and the
  man page filled in.
- `asciiverse/package.sh`: one archive per target with the binary, both
  dictionaries, the man page, README, COPYING and this file.
- `asciiverse/record-preview.py`, `asciiverse/lexter.cast`: the catalog
  preview (100x30, 52 s) and the player that recorded it, reading the
  screen through pyte: for each letter (or each letter a blank could be)
  it tries every column, preferring finished words, then runs that are
  part of a dictionary word, then low stacks, and quits into the high
  score table.
- `.github/workflows/release.yml`: static Linux in Alpine (x86_64,
  aarch64) and macOS (x86_64, arm64), plus SHA256SUMS, on a tag.
