# Repository Guidelines

## Project Structure & Module Organization

`src/` contains the C++/Qt integration layer: the puzzle catalog, the
`QQuickPaintedItem` view, and the application entry point. The QML UI lives in
`src/qml/` (`Main.qml`, `HomePage.qml`, and `PuzzlePage.qml`). Dark-mode game
overrides live in `src/night-colours.txt`; `cmake/GenerateNightColours.cmake`
resolves their symbolic `COL_*` roles against the puzzle sources.

`puzzles/` is the Simon Tatham's Puzzles git submodule. Its C puzzle engines,
HTML help, icons, and platform-specific frontends are kept upstream-style;
avoid unrelated formatting changes there. `CMakeLists.txt` assembles the
selected puzzle sources into the Qt application. `build/` is an ignored,
out-of-source build directory.

## Build, Test, and Development Commands

Initialize the dependency submodule after a fresh clone:

    git submodule update --init --recursive

Configure and build from the repository root:

    cmake -S . -B build
    cmake --build build

This requires CMake 3.22+, Qt 6.8 (Core, Gui, Qml, Quick, and QuickControls2),
and Kirigami at runtime. Launch the built application with
`./build/kpuzzlesapp`. Re-run `cmake --build build` after source changes.

There is no configured automated test target or formatter. Treat a successful
build and a manual smoke test as the current validation baseline.

## Coding Style & Naming Conventions

Use four-space indentation, K&R braces, and clear, descriptive names. C++
types and QML components use `PascalCase`; methods, properties, and local
variables use `camelCase`; puzzle identifiers and C source filenames remain
lowercase (for example, `blackbox.c`). Keep Qt signal/slot and model-role
patterns consistent with the surrounding code. No repository-wide lint or
format command is configured, so make focused, idiomatic changes.

## Testing Guidelines

For UI changes, launch the app and verify puzzle search, navigation, puzzle
interaction, timer behavior, and light/dark palette changes where relevant.
For puzzle-engine or catalog changes, confirm the affected puzzle appears,
opens, and behaves correctly; also perform a clean configure/build when CMake
source lists or generated resources change.

## Commit & Pull Request Guidelines

Recent commits use short, lowercase imperative summaries, such as `add proper
actions` and `add new Kirigami frontend`. Follow that style and keep each
commit focused. Pull requests should explain the user-visible or architectural
change, identify build or manual test commands run, note submodule updates,
and include screenshots or recordings for QML/UI changes. Link the relevant
issue when one exists and call out any new Qt/Kirigami requirement.
