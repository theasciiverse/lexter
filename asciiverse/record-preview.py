"""Records Lexter's catalog preview (hub/RECORDING.md): 100x30, about 50 s.

Runs the game under `asciinema rec` in a 100x30 pseudo-terminal, reads the
screen through pyte and plays the English dictionary: for each falling
letter (and for each letter a blank could be) it tries every column,
scoring a column by the words the letter would complete there, else by the
longest run through it that is part of some dictionary word, and a little
for keeping the stacks low; then it steers the letter over and drops it.
At the end it quits, which shows the high score table. Usage, from the
repo root after asciiverse/build.sh (the binary and dictionaries in out/),
with pyte installed:

    python3 asciiverse/record-preview.py

The game runs by name from out/ on PATH, so the cast's header holds no local
path. The cast goes to asciiverse/lexter.cast.
"""
import fcntl, os, pty, random, re, select, shutil, struct, sys, tempfile, termios, time

import pyte

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
COLS, ROWS = 100, 30
SCREEN_W, SCREEN_H = 80, 24      # the game's window, centred (text.c)
PITW, PITD = 10, 21
LENGTH = 46.0                    # seconds of play before quitting

random.seed(5)

# ncurses repeats a character with REP (CSI n b) and scrolls part of the
# screen with SU and SD (CSI n S, CSI n T), which pyte does not know
_draw = pyte.Screen.draw


def _draw_remembering(self, data):
    if data:
        self.last_char = data[-1]
    _draw(self, data)


def _repeat_last(self, n=1, *rest, **kw):
    if getattr(self, "last_char", None):
        _draw(self, self.last_char * max(1, n))


def _scroll_up(self, n=1, *rest, **kw):
    """SU (CSI n S), which ncurses uses inside a scroll region."""
    top, bottom = self.margins or pyte.screens.Margins(0, self.lines - 1)
    for _ in range(max(1, n)):
        for y in range(top, bottom):
            self.buffer[y] = self.buffer[y + 1]
        self.buffer.pop(bottom, None)
    self.dirty.update(range(self.lines))


def _scroll_down(self, n=1, *rest, **kw):
    """SD (CSI n T)."""
    top, bottom = self.margins or pyte.screens.Margins(0, self.lines - 1)
    for _ in range(max(1, n)):
        for y in range(bottom, top, -1):
            self.buffer[y] = self.buffer[y - 1]
        self.buffer.pop(top, None)
    self.dirty.update(range(self.lines))


pyte.Screen.draw = _draw_remembering
pyte.Screen.repeat_last = _repeat_last
pyte.Screen.scroll_up = _scroll_up
pyte.Screen.scroll_down = _scroll_down
pyte.Stream.csi = dict(pyte.Stream.csi, b="repeat_last", S="scroll_up", T="scroll_down")


class Rec:
    def __init__(self):
        self.home = tempfile.mkdtemp(prefix="lexter-rec-")
        env = {
            "PATH": f"{os.path.join(ROOT, 'out')}:{os.environ['PATH']}",
            "HOME": self.home,
            "XDG_DATA_HOME": os.path.join(self.home, "data"),
            "USER": "player",
            "LOGNAME": "player",
            "TERM": "xterm-256color",
            "COLORTERM": "truecolor",
            "LANG": "C.UTF-8",
        }
        self.cast = os.path.join(ROOT, "asciiverse", "lexter.cast")
        self.screen = pyte.Screen(COLS, ROWS)
        self.stream = pyte.ByteStream(self.screen)
        self.pid, self.fd = pty.fork()
        if self.pid == 0:
            os.chdir(self.home)
            os.execvpe("asciinema", ["asciinema", "rec", "--overwrite", "-q", "--idle-time-limit", "2",
                "--window-size", f"{COLS}x{ROWS}", "--capture-env", "TERM,COLORTERM,LANG",
                "--title", "Lexter", "--command", "lexter", self.cast], env)
        fcntl.ioctl(self.fd, termios.TIOCSWINSZ, struct.pack("HHHH", ROWS, COLS, 0, 0))
        self.start = time.time()

    def pump(self, t):
        end = time.time() + t
        while True:
            left = end - time.time()
            r, _, _ = select.select([self.fd], [], [], max(0.0, min(0.01, left)))
            if r:
                try:
                    self.stream.feed(os.read(self.fd, 65536))
                except OSError:
                    return False
            if left <= 0:
                return True

    def key(self, k, wait=0.0):
        os.write(self.fd, k.encode() if isinstance(k, str) else k)
        self.pump(wait)

    def type(self, text, gap=0.12):
        for ch in text:
            self.key(ch, gap)

    def text(self):
        return "\n".join(self.screen.display)

    def finish(self):
        end = time.time() + 5
        while time.time() < end:
            if not self.pump(0.1):
                break
            done, _ = os.waitpid(self.pid, os.WNOHANG)
            if done:
                break
        else:
            os.kill(self.pid, 9)    # SIGKILL: nothing more is written to the cast
        try:
            os.waitpid(self.pid, 0)
        except ChildProcessError:
            pass
        shutil.rmtree(self.home, ignore_errors=True)
        print(f"{self.cast}: {os.path.getsize(self.cast) // 1024} KB, {time.time() - self.start:.0f} s",
              file=sys.stderr)




WORDS = set()
FRAGS = set()
for w in open(os.path.join(ROOT, "data", "dict.en"), encoding="latin-1").read().split():
    WORDS.add(w)
    for i in range(len(w)):
        for j in range(i + 2, min(len(w), i + 6) + 1):
            FRAGS.add(w[i:j])


def origin():
    return (ROWS - SCREEN_H) // 2, (COLS - SCREEN_W) // 2


def pit(rec):
    """The pit as pit[x][y], " " for an empty cell, from the screen."""
    oy, ox = origin()
    d = rec.screen.display
    cells = [[" "] * PITD for _ in range(PITW)]
    for y in range(PITD):
        line = d[oy + y]
        for x in range(PITW):
            if line[ox + x * 3 + 1] in "[*":
                c = line[ox + x * 3 + 2]
                cells[x][y] = "?" if c == " " else c     # "?": a blank
    return cells


def runs_through(cells, x, y):
    """The row and column runs of letters through (x, y), each with the
    index of (x, y) in it."""
    out = []
    a = x
    while a > 0 and cells[a - 1][y] != " ":
        a -= 1
    b = x
    while b < PITW - 1 and cells[b + 1][y] != " ":
        b += 1
    out.append(("".join(cells[i][y] for i in range(a, b + 1)), x - a))
    a = y
    while a > 0 and cells[x][a - 1] != " ":
        a -= 1
    b = y
    while b < PITD - 1 and cells[x][b + 1] != " ":
        b += 1
    out.append(("".join(cells[x][i] for i in range(a, b + 1)), y - a))
    return out


def value(cells, x, letter):
    """How good dropping letter into column x would be, or None. Only
    stretches through the new letter count."""
    y = 0
    if cells[x][0] != " ":
        return None
    while y + 1 < PITD and cells[x][y + 1] == " ":
        y += 1
    cells[x][y] = letter
    best = 0.0
    for run, k in runs_through(cells, x, y):
        for s, at in ((run, k), (run[::-1], len(run) - 1 - k)):
            for i in range(0, at + 1):
                for j in range(at + 1, len(s) + 1):
                    piece = s[i:j]
                    if j - i >= 4 and piece in WORDS:
                        best = max(best, 100 + j - i)
                    elif j - i >= 2 and piece in FRAGS:
                        best = max(best, (j - i) ** 2)
    cells[x][y] = " "
    return best + y * 0.6 + random.random()


def main():
    rec = Rec()
    rec.pump(2.0)
    rec.key(" ", 0.8)                            # start
    landed = pit(rec)
    end = rec.start + LENGTH
    while time.time() < end:
        rec.pump(0.05)
        t = rec.text()
        if "Press a key" in t:
            break
        now = pit(rec)
        # the falling block: the top letter of a column that was not there
        # when the last block landed
        block = None
        for x in range(PITW):
            for y in range(PITD):
                if now[x][y] != " ":
                    if landed[x][y] == " ":
                        block = (x, y, now[x][y])
                    break
            if block:
                break
        if not block:
            continue
        bx, by, letter = block
        base = [col[:] for col in now]
        base[bx][by] = " "
        letters = [letter] if letter != "?" else list("etaoinsrhld")
        choice = None
        for l in letters:
            for x in range(PITW):
                v = value(base, x, l)
                if v is not None and (choice is None or v > choice[0]):
                    choice = (v, x, l)
        if choice is None:
            rec.key(" ", 0.3)
            continue
        _, tx, tl = choice
        rec.pump(0.25)
        if letter == "?":
            rec.key(tl, 0.15)
        while bx != tx:
            rec.key("\x1bOD" if tx < bx else "\x1bOC", 0.08)   # kcub1, kcuf1
            bx += -1 if tx < bx else 1
        rec.key(" ", 0.25)
        for _ in range(40):                      # found words flash for a second
            rec.pump(0.1)
            if "*" not in "".join(rec.screen.display[origin()[0]:origin()[0] + PITD]):
                break
        rec.pump(0.2)
        landed = pit(rec)
    rec.pump(0.5)
    rec.key("Q", 1.5)                            # quit: the score, then the table
    rec.key(" ", 4.0)
    rec.key(" ", 0.5)
    rec.finish()


if __name__ == "__main__":
    main()
