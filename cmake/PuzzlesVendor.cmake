# Build inputs for the vendored Simon Tatham's Puzzles engine sources (the
# puzzles/ git submodule).  This file is include()d from the top-level
# CMakeLists.txt and therefore runs in the caller's scope, exporting the
# PUZZLES_* variables that the application target consumes.

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
include("${CMAKE_CURRENT_LIST_DIR}/GeneratePuzzleMetadata.cmake")
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

# The vendored sources compile against puzzles.h, which relies on the
# autoconf-style HAVE_STDINT_H.  The C++ sources compile with -Werror=undef,
# so mirror the header checks from the puzzles' own build setup
# (puzzles/cmake/setup.cmake) and export the resulting definition.
include(CheckIncludeFile)
check_include_file(stdint.h HAVE_STDINT_H)
if(HAVE_STDINT_H)
    set(PUZZLES_STDINT_DEFINITION "HAVE_STDINT_H=1")
else()
    set(PUZZLES_STDINT_DEFINITION "NO_STDINT_H")
endif()

# COMBINED builds every puzzle into the one engine binary; list.c registers
# the games through generated-games.h.
set(PUZZLES_COMPILE_DEFINITIONS COMBINED ${PUZZLES_STDINT_DEFINITION})

set(PUZZLES_SOURCES)
foreach(source IN LISTS PUZZLES_CORE_SOURCES PUZZLES_GAME_SOURCES)
    list(APPEND PUZZLES_SOURCES "${PUZZLES_DIR}/${source}")
endforeach()
list(APPEND PUZZLES_SOURCES "${PUZZLES_DIR}/list.c")
