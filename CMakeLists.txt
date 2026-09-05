cmake_minimum_required(VERSION 3.22)

project(kpuzzles VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)

# Kirigami is a QML dependency.  It is deliberately discovered here so that
# package managers can diagnose a missing KDE runtime dependency at configure
# time, while keeping the source tree usable on systems that only provide Qt
# for static analysis.
find_package(KF6Kirigami 6.0 QUIET)
if(NOT KF6Kirigami_FOUND)
    message(WARNING "KF6Kirigami was not found. kpuzzles will build, but the QML application needs the KDE Kirigami runtime to start.")
endif()

qt_standard_project_setup(REQUIRES 6.8)

set(PUZZLES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/puzzles")

# These are the platform-independent sources used by puzzles' own Unix
# frontends.  gtk.c/windows.c/nullfe.c and all auxiliary programs are not part
# of the Qt frontend.  Keeping the list explicit prevents a helper with its
# own main() from accidentally becoming part of the application.
set(PUZZLES_CORE_SOURCES
    combi.c
    divvy.c
    draw-poly.c
    drawing.c
    dsf.c
    findloop.c
    grid.c
    latin.c
    laydomino.c
    loopgen.c
    malloc.c
    matching.c
    midend.c
    misc.c
    penrose.c
    penrose-legacy.c
    printing.c
    ps.c
    random.c
    sort.c
    spectre.c
    tdq.c
    tree234.c
    version.c
    hat.c
)

set(PUZZLES_GAME_SOURCES
    blackbox.c
    bridges.c
    cube.c
    dominosa.c
    fifteen.c
    filling.c
    flip.c
    flood.c
    galaxies.c
    guess.c
    inertia.c
    keen.c
    lightup.c
    loopy.c
    magnets.c
    map.c
    mines.c
    mosaic.c
    net.c
    netslide.c
    palisade.c
    pattern.c
    pearl.c
    pegs.c
    range.c
    rect.c
    samegame.c
    signpost.c
    singles.c
    sixteen.c
    slant.c
    solo.c
    tents.c
    towers.c
    tracks.c
    twiddle.c
    undead.c
    unequal.c
    unruly.c
    untangle.c
)

set(PUZZLES_GAME_NAMES
    blackbox bridges cube dominosa fifteen filling flip flood galaxies guess
    inertia keen lightup loopy magnets map mines mosaic net netslide palisade
    pattern pearl pegs range rect samegame signpost singles sixteen slant solo
    tents towers tracks twiddle undead unequal unruly untangle
)

# list.c includes this generated header when COMBINED is enabled.
set(GENERATED_PUZZLES_INCLUDE "${CMAKE_CURRENT_BINARY_DIR}/generated-puzzles")
file(MAKE_DIRECTORY "${GENERATED_PUZZLES_INCLUDE}")
set(GENERATED_GAMES_HEADER "${GENERATED_PUZZLES_INCLUDE}/generated-games.h")
file(WRITE "${GENERATED_GAMES_HEADER}" "")
foreach(game_name IN LISTS PUZZLES_GAME_NAMES)
    file(APPEND "${GENERATED_GAMES_HEADER}" "GAME(${game_name})\n")
endforeach()

set(PUZZLES_SOURCES)
foreach(source IN LISTS PUZZLES_CORE_SOURCES PUZZLES_GAME_SOURCES)
    list(APPEND PUZZLES_SOURCES "${PUZZLES_DIR}/${source}")
endforeach()
list(APPEND PUZZLES_SOURCES "${PUZZLES_DIR}/list.c")

qt_add_executable(kpuzzles
    src/main.cpp
    src/PuzzleCatalog.cpp
    src/PuzzleCatalog.h
    src/PuzzleView.cpp
    src/PuzzleView.h
    ${PUZZLES_SOURCES}
)

target_compile_definitions(kpuzzles PRIVATE COMBINED)
target_include_directories(kpuzzles PRIVATE
    "${PUZZLES_DIR}"
    "${GENERATED_PUZZLES_INCLUDE}"
)

qt_add_qml_module(kpuzzles
    URI kpuzzles
    VERSION 1.0
    QML_FILES
        Main.qml
        HomePage.qml
        PuzzlePage.qml
)

target_link_libraries(kpuzzles PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    m
)

if(TARGET KF6::Kirigami)
    target_link_libraries(kpuzzles PRIVATE KF6::Kirigami)
endif()

set_target_properties(kpuzzles PROPERTIES
    # qt_add_qml_module uses a build-time directory named after the target;
    # keep the executable name distinct from it.
    OUTPUT_NAME kpuzzlesapp
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

install(TARGETS kpuzzles
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)
