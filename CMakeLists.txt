cmake_minimum_required(VERSION 3.22)

project(kpuzzles VERSION 0.1.0 LANGUAGES C CXX)

include(GNUInstallDirs)

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

set(PUZZLES_GAME_NAMES
    blackbox bridges cube dominosa fifteen filling flip flood galaxies guess
    inertia keen lightup loopy magnets map mines mosaic net netslide palisade
    pattern pearl pegs range rect samegame signpost singles sixteen slant solo
    tents towers tracks twiddle undead unequal unruly untangle
)

set(PUZZLES_HELP_FILES)
set(PUZZLES_THUMBNAIL_FILES)
set(PUZZLES_GAME_SOURCES)
foreach(game_name IN LISTS PUZZLES_GAME_NAMES)
    list(APPEND PUZZLES_GAME_SOURCES "${game_name}.c")
    list(APPEND PUZZLES_HELP_FILES
        "${PUZZLES_DIR}/html/${game_name}.html")
    list(APPEND PUZZLES_THUMBNAIL_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/data/thumbnails/${game_name}.png")
endforeach()

# list.c includes this generated header when COMBINED is enabled.
set(GENERATED_PUZZLES_INCLUDE "${CMAKE_CURRENT_BINARY_DIR}/generated-puzzles")
file(MAKE_DIRECTORY "${GENERATED_PUZZLES_INCLUDE}")
set(GENERATED_GAMES_HEADER "${GENERATED_PUZZLES_INCLUDE}/generated-games.h")
set(generated_games "")
foreach(game_name IN LISTS PUZZLES_GAME_NAMES)
    string(APPEND generated_games "GAME(${game_name})\n")
endforeach()
set(generated_games_temporary "${GENERATED_GAMES_HEADER}.tmp")
file(WRITE "${generated_games_temporary}" "${generated_games}")
configure_file(
    "${generated_games_temporary}"
    "${GENERATED_GAMES_HEADER}"
    COPYONLY
)
file(REMOVE "${generated_games_temporary}")

# Puzzle display names and summaries belong to the upstream puzzle catalog.
# Generate a compile-time table from that source so the frontend cannot drift
# from the bundled version of puzzles.
include(cmake/GeneratePuzzleMetadata.cmake)
set(PUZZLES_UPSTREAM_CMAKE "${PUZZLES_DIR}/CMakeLists.txt")
set(GENERATED_PUZZLE_METADATA_HEADER
    "${GENERATED_PUZZLES_INCLUDE}/generated-puzzle-metadata.h")
generate_puzzle_metadata(
    "${GENERATED_PUZZLE_METADATA_HEADER}"
    "${PUZZLES_UPSTREAM_CMAKE}"
    ${PUZZLES_GAME_NAMES}
)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${PUZZLES_UPSTREAM_CMAKE}"
)

# The dark palette is maintained using symbolic COL_* names. Resolve those
# names against the bundled puzzle sources at configure time so an upstream
# enum change cannot silently apply an override to the wrong colour.
include(cmake/GenerateNightColours.cmake)
set(NIGHT_COLOURS_DATA "${CMAKE_CURRENT_SOURCE_DIR}/src/night-colours.txt")
set(GENERATED_NIGHT_COLOURS_HEADER
    "${GENERATED_PUZZLES_INCLUDE}/generated-night-colours.h")
generate_night_colours(
    "${GENERATED_NIGHT_COLOURS_HEADER}"
    "${NIGHT_COLOURS_DATA}"
    "${PUZZLES_DIR}"
)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${NIGHT_COLOURS_DATA}"
)
foreach(game_source IN LISTS PUZZLES_GAME_SOURCES)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${PUZZLES_DIR}/${game_source}"
    )
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
    src/SessionState.cpp
    src/SessionState.h
    src/night-colours.txt
    ${GENERATED_PUZZLE_METADATA_HEADER}
    ${GENERATED_NIGHT_COLOURS_HEADER}
    ${PUZZLES_SOURCES}
)

target_compile_definitions(kpuzzles PRIVATE COMBINED)
target_include_directories(kpuzzles PRIVATE
    "${PUZZLES_DIR}"
    "${GENERATED_PUZZLES_INCLUDE}"
)

qt_add_resources(kpuzzles puzzle_help
    PREFIX "/puzzles-help"
    BASE "${PUZZLES_DIR}/html"
    FILES ${PUZZLES_HELP_FILES}
)

qt_add_resources(kpuzzles application_icon
    PREFIX "/icons"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/data/icons"
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/data/icons/org.kde.kpuzzles.svg"
)

qt_add_resources(kpuzzles puzzle_thumbnails
    PREFIX "/puzzle-thumbnails"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/data/thumbnails"
    FILES ${PUZZLES_THUMBNAIL_FILES}
)

qt_add_qml_module(kpuzzles
    URI kpuzzles
    VERSION 1.0
    QML_FILES
        src/qml/Main.qml
        src/qml/HomePage.qml
        src/qml/PuzzlePage.qml
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
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

if(UNIX AND NOT APPLE)
    install(FILES data/org.kde.kpuzzles.desktop
        DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
    )
    install(FILES data/icons/org.kde.kpuzzles.svg
        DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps"
    )
endif()
