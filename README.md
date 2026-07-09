# Kilo — A Tiny Terminal Text Editor

> Built in C. No libraries. No bloat. Just under a thousand lines.

Kilo is a terminal-based text editor that started as a fork of [antirez/kilo](https://github.com/antirez/kilo) — the legendary editor written in under 1K lines of C. This version is stripped down, cleaned up, and built from the ground up as a learning project.

It doesn't depend on ncurses, Lua, or anything fancy. Just raw VT100 escape sequences and POSIX syscalls.

---

## What it looks like

```
┌──────────────────────────────────────────────────────────────┐
│  ~                                                           │
│  ~            Kilo editor -- version 0.0.1                   │
│  ~                                                           │
│  ~                                                           │
│  ├───────────────────────────────────────────────────────────┤
│  │ filename.c - 142 lines (modified)     c | 23/142         │
│  └───────────────────────────────────────────────────────────┘
│  HELP: Ctrl-O = save | Ctrl-Q = quit                         │
└──────────────────────────────────────────────────────────────┘
```

---

## Features

| Key            | Action                          |
| -------------- | ------------------------------- |
| `Arrow keys`   | Move cursor                     |
| `Home` / `End` | Jump to start / end of line     |
| `PgUp` / `PgDn`| Scroll page up / down            |
| `Ctrl-O`       | Save file to disk               |
| `Ctrl-F`       | Search within file              |
| `Ctrl-Q`       | Quit (asks twice if unsaved)    |
| `Backspace`    | Delete character before cursor  |
| `Delete`       | Delete character under cursor   |

Also includes:
- Syntax highlighting for C-family languages (numbers)
- Tabs expanded to spaces
- A status bar showing filename, line count, and cursor position
- Dirty file tracking — won't let you accidentally lose changes

---

## Build & Install

```sh
make          # builds the 'kilo' binary
make install  # copies it to ~/.local/bin
```

Make sure `~/.local/bin` is in your `$PATH`:

```sh
export PATH="$HOME/.local/bin:$PATH"
```

---

## Usage

```sh
kilo somefile.c
```

If the file exists, it opens. If it doesn't, that's fine too — start typing and save when you're done.

---

## Why?

Because every developer should write a text editor at least once.

This project was my way of understanding:
- How terminals actually work (raw mode, escape sequences)
- How to read and write files at the syscall level
- What goes into building something usable from scratch

No frameworks. No dependencies. Just C, a terminal, and some stubbornness.

---

## How fast is it?

I/O is one place where simplicity really pays off. Here's how Kilo compares to Vim and Nano at reading files:

| File size | **Kilo** (I/O only) | Vim (ex mode) | Nano          |
| --------- | ------------------- | ------------- | ------------- |
| **1 KB**  | **< 0.01 ms**       | 6.4 ms        | 274 ms        |
| **1 MB**  | **9.6 ms**          | 9.3 ms        | 275 ms        |
| **10 MB** | **93.8 ms**         | 40.2 ms       | 276 ms        |
| **50 MB** | **469 ms**          | 157 ms        | 525 ms        |

**Why Kilo is fast for small files** — because there's nothing to load. Plugins, config files, syntax engines? None of that exists here. It just opens the file and shows it to you.

Raw throughput clocks in at **~104 MB/s** for reads and **~800 MB/s** for writes (page-cached). The entire I/O path is a few dozen lines of C — `fopen` / `getline` for reading, `open` / `write` / `ftruncate` for saving.

*(Benchmarks ran on a Linux 6.8 system with SSD storage. Results averaged over 5 runs after 3 warmup iterations. Source: `bench_kilo.c` in the repo.)*

---

## Project structure

```
├── kilo.c       # the entire editor (~911 lines)
├── bench_kilo.c # I/O benchmark harness
├── Makefile
└── README.md
```

---

## Shamelessly inspired by

- [antirez/kilo](https://github.com/antirez/kilo) — the original
- [Build Your Own Text Editor](https://viewsourcecode.org/snaptoken/kilo/) — the tutorial that walks through it line by line
- A bunch of late nights and segfaults

---

## License

BSD 2-Clause — same as the original. Go build stuff with it.
