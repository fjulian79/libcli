# Line Endings, Terminals and Telnet

This document explains what happens to the Enter key and to line endings on
their way between a keyboard and libCli, and back to the screen. It covers what
Linux does, what the common terminal programs do, what Telnet requires, how
libCli handles all of this and which settings work.

The topic looks trivial and is not. Most problems are caused by a program in
the middle "helpfully" translating bytes, in a way that nobody asked for.

## Table of Contents

- [The Bytes](#the-bytes)
- [What the Enter Key Sends](#what-the-enter-key-sends)
- [What Linux Does: the TTY Line Discipline](#what-linux-does-the-tty-line-discipline)
- [Terminal Programs](#terminal-programs)
  - [picocom](#picocom)
  - [The PlatformIO Monitor (miniterm)](#the-platformio-monitor-miniterm)
  - [Other Terminal Programs](#other-terminal-programs)
- [Telnet](#telnet)
- [How libCli Handles It](#how-libcli-handles-it)
- [Pitfalls](#pitfalls)
- [Recommended Settings](#recommended-settings)
- [Lessons Learned](#lessons-learned)

## The Bytes

| Name | C    | Hex    | Meaning |
|------|------|--------|---------|
| CR   | `\r` | `0x0D` | Carriage return, move the cursor to the first column |
| LF   | `\n` | `0x0A` | Line feed, move the cursor one line down |
| NUL  | `\0` | `0x00` | Null byte, no meaning on a terminal |

The names come from teletypes: returning the carriage and feeding the paper
were two separate mechanical actions. A "new line" on the screen is therefore
CR followed by LF.

Unix stores text with LF only, Windows with CR LF. That is about files. On a
serial line the question is a different one: which bytes does the terminal send
when Enter is pressed, and which bytes does it expect to start a new line?

## What the Enter Key Sends

A terminal in raw mode, i.e. one that passes every key on as it is, sends
**CR** for the Enter key. This is true for every common terminal emulator on
Linux and macOS. Pressing Ctrl+J sends LF, Ctrl+M sends CR, which shows that
Enter is the same as Ctrl+M.

So the natural behavior is: **Enter sends CR**. Everything else is caused by a
translation somewhere.

## What Linux Does: the TTY Line Discipline

On Linux (and macOS, BSD) a serial port or a terminal is a *tty*. Between the
hardware and the program reading from it, the kernel runs the *line
discipline*. It is configured by the termios flags, see `stty -a`. The relevant
ones are:

| Flag     | Direction | Effect |
|----------|-----------|--------|
| `ICRNL`  | input     | A received CR is translated to LF |
| `INLCR`  | input     | A received LF is translated to CR |
| `IGNCR`  | input     | A received CR is dropped |
| `ICANON` | input     | Canonical mode: the kernel collects a whole line, handles backspace, and hands it out when the line is complete |
| `ECHO`   | input     | The kernel echoes the received characters |
| `OPOST`  | output    | Enables output processing at all |
| `ONLCR`  | output    | An LF written by a program is sent as CR LF |

The defaults are `ICRNL`, `ICANON`, `ECHO`, `OPOST` and `ONLCR`. As a result:

- A program reading from the tty only ever sees LF as line end, no matter that
  the terminal sent CR.
- A program writes LF only, the kernel turns it into CR LF for the terminal.
- Line editing and echo are done by the kernel in canonical mode, e.g. while
  `login` asks for the user name and the password.

This is why a Raspberry Pi on a serial console "just works": picocom sends CR,
the kernel turns it into LF, bash and `login` see LF. Nothing in user space has
to care about line endings at all.

It also shows what goes wrong with CR LF. With `ICRNL` a received CR LF becomes
LF LF, i.e. two Enter presses. In bash this only causes an additional empty
prompt, but `login` takes the second one as an empty password. That is the
reason why terminal programs send CR only by default.

A microcontroller has no kernel and no line discipline. libCli is the one who
has to do this job.

The terminal program itself runs in a terminal window on the PC, which is a
tty as well, with termios flags of its own. Whether an LF sent by the 
microcontroller firmware is displayed as a new line starting in the first column
depends on how the terminal program configures this local tty:

- picocom switches it to raw mode, which disables `OPOST` and with it
  `ONLCR`. A bare LF moves the cursor one line down but not back to the first
  column, the output becomes a staircase. This is why picocom needs
  `--imap lfcrlf` for a microcontroller.
- miniterm only disables `ICANON`, `ECHO` and `ISIG` and leaves the output
  processing on. `ONLCR` of the local tty adds the CR, i.e. a received LF is
  displayed as CR LF. This is the same as `--imap lfcrlf` of picocom, just
  done by the kernel of the PC instead of the terminal program.

Either way, a firmware ending its lines with LF only is displayed correctly,
which is why `printf("...\n")` works on a microcontroller although it never
sends a CR.

## Terminal Programs

### picocom

picocom passes the keys on as they are, so Enter sends CR. It translates only
if told to, separately per direction:

| Option            | Direction           | Effect |
|-------------------|---------------------|--------|
| `--omap crlf`     | keyboard to device  | CR is sent as LF |
| `--omap crcrlf`   | keyboard to device  | CR is sent as CR LF |
| `--imap lfcrlf`   | device to screen    | A received LF is displayed as CR LF |

For a microcontroller the following is a good choice:

```bash
picocom -b 115200 --imap lfcrlf /dev/tty.usbserial-XXXX
```

Enter sends CR, which is what libCli expects. The firmware may end its lines
with LF only, picocom adds the CR for the display. For a Raspberry Pi
`--imap lfcrlf` is not needed, its kernel does the same by `ONLCR`.

### The PlatformIO Monitor (miniterm)

`pio device monitor` is a wrapper around miniterm of pyserial. This is true for
the original PlatformIO as well as for the pioarduino fork, the monitor code is
the same in both and has been for years.

Two layers translate line endings on the way through the monitor: miniterm
itself, configured by the end of line mode `monitor_eol` in `platformio.ini`
respectively `--eol`, and the local tty, i.e. the tty of the terminal window
miniterm runs in. The serial port does not matter here, pyserial opens it in
raw mode and the bytes pass it unchanged.

```
               miniterm                        local tty
MCU  ────────▶ monitor_eol, receive ─────────▶ ONLCR: LF → CR LF ───▶ screen
               CR → LF, only for mode CR

MCU  ◀──────── monitor_eol, send   ◀───────── ICRNL: CR → LF    ◀─── keyboard
               LF → CR LF, CR or LF
```

Unlike picocom, miniterm leaves the local tty mostly as it is, it only
disables `ICANON`, `ECHO` and `ISIG`. So `ICRNL` and `ONLCR` are still on and
do their job, in addition to miniterm.

**Receiving, from the microcontroller to the screen:**

1. Only with `monitor_eol = CR` miniterm replaces every received CR by LF.
   With the other modes it passes the received data on unchanged.
2. The local tty turns each LF into CR LF by `ONLCR`, like picocom does with
   `--imap lfcrlf`. This is done by the kernel of the PC and is not an option
   of miniterm.

**Sending, from the keyboard to the microcontroller:**

1. The local tty turns the CR of the Enter key into LF by `ICRNL`, before
   miniterm reads it.
2. miniterm translates this LF according to `monitor_eol` and sends the
   result to the microcontroller.

The result for each mode, the bytes miniterm passes on to the local tty,
which then turns each LF into CR LF:

| `monitor_eol`    | Keyboard Enter sends | MCU sends CR | MCU sends LF | MCU sends CR LF |
|------------------|----------------------|--------------|--------------|-----------------|
| `CRLF` (default) | CR LF                | CR           | LF           | CR LF           |
| `CR`             | CR                   | **LF**       | LF           | **LF LF**       |
| `LF`             | LF                   | CR           | LF           | CR LF           |

- `CRLF` displays everything correctly, but sends two bytes for every Enter,
  see [Pitfalls](#pitfalls).
- `CR` sends exactly what would be expected, but changes the received data.
  Everything which relies on a CR to return to the start of the current line
  breaks, e.g. a line which is cleared and redrawn, or a status line which is
  overwritten continuously. A CR LF sent by the firmware, e.g. by
  `Serial.println()`, shows up as an additional empty line.
- `LF` displays everything correctly, but the Enter key sends LF instead of the
  expected CR, see [Pitfalls](#pitfalls).

`monitor_raw = yes` disables the processing of the received data by miniterm,
i.e. the replacement of CR in step 1 of the receive direction and all monitor
filters, e.g. `esp32_exception_decoder`. The sent data is still translated,
and `ONLCR` of the local tty still applies as it is done by the kernel.
Together with `monitor_eol = CR` the monitor behaves like picocom with
`--imap lfcrlf`: Enter sends CR, and a CR from the firmware returns to the
first column.

### Other Terminal Programs

screen, minicom and tio send CR for Enter by default, like picocom. They offer
mapping options of their own, check their documentation before relying on
them.

Windows terminal programs such as PuTTY send CR by default as well. Some
programs offer "send CR LF" or "implicit CR/LF" options, these are the usual
source of a second byte.

## Telnet

Telnet does not transport the bytes of a terminal as they are, it defines a
*Network Virtual Terminal* (NVT), see RFC 854. Its rules for line endings are:

- **CR LF** is a new line.
- **CR NUL** is a carriage return alone.
- A CR must never be sent on its own, it is always followed by LF or NUL.

So a Telnet client sends either CR LF or CR NUL for the Enter key, depending on
the client and its mode. Two bytes are required by the protocol here, this is
not a quirk of a client.

Telnet has more to offer:

- **IAC** (Interpret As Command, `0xFF`) starts a command, e.g. the option
  negotiation. A data byte `0xFF` is sent as `0xFF 0xFF`. A client usually
  starts negotiating right after connecting, at least when connecting to port
  23. These bytes must not end up as user input.
- **Line mode and character mode.** Without negotiation a client works line by
  line: it collects and edits the line locally, echoes it locally and sends it
  on Enter. Cursor keys, a command history or tab completion of the server can
  not work this way. The server switches the client to character mode by
  sending `IAC WILL ECHO` (the server echoes) and `IAC WILL SUPPRESS-GO-AHEAD`.
  The client then sends every key immediately and stops echoing locally. Enter
  is still sent as CR LF or CR NUL.

On Linux `telnetd` sits between the network and the tty. It handles IAC,
negotiates the options and turns the NVT line ending into a single CR before
passing it on to the line discipline. This is exactly the job a Telnet server
on a microcontroller has to do itself, in front of libCli.

## How libCli Handles It

**Input:**

- **CR** is Enter, it executes the command.
- **LF** at the start of a line is dropped, which covers the second byte of
  CR LF. An LF within a line is stored as an ordinary character.
- **NUL** is dropped everywhere, which covers the second byte of CR NUL.

**Output:**

- A new line is written as LF only. A terminal program adds the CR, either by
  an option like picocom `--imap lfcrlf` or by `ONLCR` of the local tty.
- `clearLine()` erases the line by `ESC [ 999 D` (cursor 999 columns back,
  it stops at the margin) followed by `ESC [ 2 K` (erase the line). It does
  not use CR, so the line is cleared correctly even if the terminal program
  translates CR to LF.

**History of this topic in libCli:**

- Up to 4.2.0 the command history only restored the last command on an empty
  line by writing it behind the prompt. No CR was written, `monitor_eol = CR`
  worked without any problem.
- 4.3.0 introduced the full command history. Each step through the history
  clears the line and redraws the prompt. `clearLine()` used
  `CR ESC [ 2 K` then, with `monitor_eol = CR` the CR arrived as LF and each
  step was printed on a new line.
- `clearLine()` no longer writes a CR since 4.8.1.

## Pitfalls

**A command reading input sees the second byte.** A command runs within the
call which processed the CR. If the terminal sends CR LF or CR NUL, the second
byte is still in the stream at this time. A command which reads from the stream
itself, e.g. to ask for a password or to wait for any key, receives it first.
libCli never sees this byte, so its rules above do not help here.

Avoid it by making sure only CR arrives: configure the terminal program to
send CR only, and convert the Telnet line ending in the Telnet layer, see
below. If that is not possible, the command has to skip LF and NUL itself.

**Overwriting a line by CR in the application.** A status line printed
continuously and overwritten by a leading CR breaks with `monitor_eol = CR`
just like the history did, each update ends up on a new line. Use
`ESC [ 999 D` instead of CR, like `clearLine()` does.

**`monitor_eol = LF`.** libCli executes a command on CR only. With LF as line
end nothing is executed, the LF is stored as input.

## Recommended Settings

| Connection | Setting |
|------------|---------|
| picocom | `picocom -b <baud> --imap lfcrlf <device>`, no `--omap` |
| PlatformIO monitor | `monitor_eol = CR` with a libCli version whose `clearLine()` writes no CR. With older versions add `monitor_raw = yes` |
| Telnet | The Telnet server removes the IAC sequences and turns CR LF and CR NUL into CR before the data reaches libCli. Negotiate character mode for cursor keys, history and tab completion, and enable the echo of libCli |
| Application output | LF for a new line, `ESC [ 999 D` to return to the start of the current line, never a bare CR |

With these settings libCli and every command only ever see a single CR for
Enter, on every connection.

A minimal filter for the Telnet line ending, as part of a `Stream` wrapping the
client socket:

```cpp
/* Called for every data byte after the IAC sequences have been removed.
 * Returns false if the byte is to be dropped. */
bool TelnetStream::filterEol(int c) {
    bool drop = LastWasCr && (c == '\n' || c == '\0');

    LastWasCr = (c == '\r');
    return !drop;
}
```

Since every read goes through the wrapper, it does not matter whether libCli
or a command reads the byte, and it does not matter when the second byte
arrives.

## Lessons Learned

- Enter sends CR. Every other line ending on the input side is a translation
  done by some program in the middle.
- On Linux the kernel normalizes line endings for every program, on a
  microcontroller nobody does unless the firmware does it.
- Telnet requires two bytes for Enter. Handle that in the Telnet layer, like
  `telnetd` does, not in the command interpreter.
- miniterm applies its end of line mode to the sent data, the mode `CR` to
  the received data as well. `monitor_eol = CR` fixes the sending side and
  breaks every CR sent by the firmware.
- Do not use a bare CR in output that has to survive such a terminal, use an
  escape sequence to move the cursor.
- To find out what really arrives, print the received bytes in hex instead of
  guessing.
