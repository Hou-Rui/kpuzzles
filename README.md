# KPuzzles

KPuzzles is a desktop frontend for [Simon Tatham's Portable Puzzle
Collection](https://www.chiark.greenend.org.uk/~sgtatham/puzzles/), built with
Qt 6 Quick/QML and KDE Kirigami. It bundles all 40 puzzles from the collection
(Net, Mines, Solo, Untangle, and more) in a single application with a modern,
adaptive interface.

## Features

- All puzzles from the collection in one catalog, with search
- Built-in per-puzzle help pages from the upstream documentation
- Automatic session restore across restarts
- Dark-mode friendly puzzle colours via symbolic colour overrides

## Requirements

- CMake 3.22 or newer
- Qt 6.8 or newer (Core, Gui, Qml, Quick, QuickControls2, Widgets,
  LabsPlatform)
- KDE Kirigami 6 runtime
- A C and C++ compiler (C99 / C++20)

## Building

The `puzzles/` directory is a git submodule containing the upstream C puzzle
engines. Initialize it after a fresh clone:

    git submodule update --init --recursive

Configure and build from the repository root:

    cmake -S . -B build
    cmake --build build

Launch the application:

    ./build/kpuzzlesapp

## Repository Layout

- `src/` — C++/Qt integration layer (puzzle catalog, puzzle view, session
  state) and the QML UI in `src/qml/`
- `puzzles/` — Simon Tatham's Puzzles git submodule (kept upstream-style)
- `cmake/` — CMake helpers that generate puzzle metadata and dark-mode
  colours from symbolic `COL_*` roles
- `data/` — application icon, desktop file, and puzzle thumbnails
