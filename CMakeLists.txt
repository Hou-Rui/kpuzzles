cmake_minimum_required(VERSION 3.22)

project(kpuzzles VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FeatureSummary)

# The Extra CMake Modules package provides the KDE build system modules
# included below, matching how the official Kirigami applications are built.
find_package(ECM 6.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMFindQmlModule)
include(ECMQmlModule)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    LabsPlatform
    Qml
    Quick
    QuickControls2
    Widgets
)

find_package(KF6 REQUIRED COMPONENTS
    Kirigami
    QQC2DesktopStyle
    IconThemes
)

ecm_find_qmlmodule(org.kde.kirigami REQUIRED)

# The vendored Simon Tatham's Puzzles engines (the puzzles/ git submodule)
# supply the game implementations.  The build inputs assembled from the
# submodule are defined in their own module and included into this scope.
include(cmake/PuzzlesVendor.cmake)

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

# Compile definitions and include paths for the vendored puzzle engines.
target_compile_definitions(kpuzzles PRIVATE ${PUZZLES_COMPILE_DEFINITIONS})
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
    Qt6::LabsPlatform
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Widgets
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
    RUNTIME DESTINATION "${KDE_INSTALL_BINDIR}"
)

if(UNIX AND NOT APPLE)
    install(FILES data/org.kde.kpuzzles.desktop
        DESTINATION "${KDE_INSTALL_APPDIR}"
    )
    install(FILES data/icons/org.kde.kpuzzles.svg
        DESTINATION "${KDE_INSTALL_FULL_ICONDIR}/hicolor/scalable/apps"
    )
endif()

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
